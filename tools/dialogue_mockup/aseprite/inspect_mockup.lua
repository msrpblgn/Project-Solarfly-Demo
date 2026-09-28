-- Inspect dialogue_behavior_mockup.aseprite layers, frames, and tags.

local configPath = app.fs.joinPath(
  app.fs.currentPath,
  "assets_src/ui/dialogue/build/mockup_config.json"
)

local function read_json(path)
  local file = io.open(path, "r")
  if not file then
    error("Could not open config: " .. path)
  end
  local text = file:read("*a")
  file:close()
  return json.decode(text)
end

local config = read_json(configPath)
local sprite = app.open(config.output_aseprite)
app.activeSprite = sprite

print("ASEPRITE_INSPECTION_BEGIN")
print("frame_count=" .. #sprite.frames)
print("layer_count=" .. #sprite.layers)

for i, layer in ipairs(sprite.layers) do
  print("layer_" .. i .. "=" .. layer.name)
end

for i, tag in ipairs(sprite.tags) do
  print(
    "tag_" .. i .. "=" .. tag.name .. " from=" .. tag.fromFrame.frameNumber .. " to=" .. tag.toFrame.frameNumber
  )
end

print("ASEPRITE_INSPECTION_END")
