"""GIF preview timing: 60 fps logical timeline with cumulative 10 ms quantization."""

from __future__ import annotations

from dataclasses import dataclass

from PIL import Image

from tools.dialogue_mockup.constants import (
    LOGICAL_FRAME_MS,
    MOTION_FPS,
    NATIVE_PREVIEW_SPEED,
    REVIEW_4X_SPEED,
)

# GIF Graphics Control Extension delay unit.
GIF_DELAY_UNIT_MS = 10


@dataclass(frozen=True)
class GifExportResult:
    path: str
    logical_frame_count: int
    encoded_frame_count: int
    intended_duration_ms: float
    expected_encoded_duration_ms: int
    measured_duration_ms: int
    speed_multiplier: float
    frame_mapping: list[dict[str, int]]


def logical_timeline_ms(frame_count: int, speed_multiplier: float = NATIVE_PREVIEW_SPEED) -> list[float]:
    """Cumulative end timestamps for logical frames [1..N], starting after frame 0."""
    step = LOGICAL_FRAME_MS * speed_multiplier
    return [step * (i + 1) for i in range(frame_count)]


def intended_duration_ms(frame_count: int, speed_multiplier: float = NATIVE_PREVIEW_SPEED) -> float:
    return LOGICAL_FRAME_MS * frame_count * speed_multiplier


def quantize_timestamp_ms(timestamp_ms: float) -> int:
    """Round cumulative time to the nearest GIF delay unit (10 ms)."""
    return int(round(timestamp_ms / GIF_DELAY_UNIT_MS)) * GIF_DELAY_UNIT_MS


def collapse_identical_frames(
    frames: list[Image.Image],
) -> tuple[list[Image.Image], list[tuple[int, int]]]:
    """
    Collapse consecutive identical RGB frames.

    Returns unique frames and inclusive logical index ranges [(start, end), ...].
    """
    if not frames:
        return [], []
    rgb = [frame.convert("RGB") for frame in frames]
    unique: list[Image.Image] = [rgb[0]]
    ranges: list[tuple[int, int]] = [(0, 0)]
    for index, frame in enumerate(rgb[1:], start=1):
        if frame.tobytes() == unique[-1].tobytes():
            start, _ = ranges[-1]
            ranges[-1] = (start, index)
        else:
            unique.append(frame)
            ranges.append((index, index))
    return unique, ranges


def encoded_delays_ms(
    frame_count: int,
    ranges: list[tuple[int, int]],
    speed_multiplier: float,
) -> list[int]:
    """
    Derive GIF frame delays from quantized cumulative timestamps.

    Quantizing cumulative ends (not each per-frame duration) prevents truncation drift.
    """
    ends = logical_timeline_ms(frame_count, speed_multiplier)
    delays: list[int] = []
    prev_q = 0
    for start, end in ranges:
        q_end = quantize_timestamp_ms(ends[end])
        delay = q_end - prev_q
        if delay < GIF_DELAY_UNIT_MS:
            # GIF delay 0 is often treated as a browser default; keep at least one unit.
            delay = GIF_DELAY_UNIT_MS
            q_end = prev_q + delay
        delays.append(delay)
        prev_q = q_end
    return delays


def expected_encoded_duration_ms(frame_count: int, speed_multiplier: float) -> int:
    """Expected total GIF duration after cumulative quantization (no collapse)."""
    return quantize_timestamp_ms(intended_duration_ms(frame_count, speed_multiplier))


def measure_gif_durations(path) -> list[int]:
    with Image.open(path) as image:
        durations: list[int] = []
        index = 0
        while True:
            try:
                image.seek(index)
            except EOFError:
                break
            durations.append(int(image.info.get("duration", 0)))
            index += 1
        return durations


def measure_gif_total_duration_ms(path) -> int:
    return sum(measure_gif_durations(path))


def count_gif_frames(path) -> int:
    return len(measure_gif_durations(path))


def save_preview_gif(
    frames: list[Image.Image],
    path,
    speed_multiplier: float,
) -> GifExportResult:
    """
    Write a preview GIF at the documented speed multiplier.

    Identical consecutive frames collapse; their display times accumulate via
    cumulative timestamp quantization (GIF 10 ms units).
    """
    if not frames:
        raise ValueError("cannot save empty GIF")

    unique, ranges = collapse_identical_frames(frames)
    delays = encoded_delays_ms(len(frames), ranges, speed_multiplier)
    unique[0].save(
        path,
        save_all=True,
        append_images=unique[1:],
        duration=delays,
        loop=0,
        disposal=2,
    )

    measured = measure_gif_total_duration_ms(path)
    mapping = [
        {
            "encoded_index": i,
            "logical_start": start,
            "logical_end": end,
            "duration_ms": delays[i],
        }
        for i, (start, end) in enumerate(ranges)
    ]
    return GifExportResult(
        path=str(path),
        logical_frame_count=len(frames),
        encoded_frame_count=len(unique),
        intended_duration_ms=intended_duration_ms(len(frames), speed_multiplier),
        expected_encoded_duration_ms=sum(delays),
        measured_duration_ms=measured,
        speed_multiplier=speed_multiplier,
        frame_mapping=mapping,
    )
