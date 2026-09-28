-- Export flattened reference frames (excluding GUIDES_do_not_export).

local configPath = app.fs.joinPath(
  app.fs.currentPath,
  "assets_src/ui/dialogue/build/mockup_config.json"
)
local outDir = app.fs.joinPath(
  app.fs.currentPath,
  "assets_src/ui/dialogue/build/roundtrip"
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

local function should_export_layer(name)
  return name ~= "GUIDES_do_not_export"
end

local function flatten_frame(sprite, frame)
  local image = Image(sprite.width, sprite.height, sprite.colorMode)
  image:clear()
  for _, layer in ipairs(sprite.layers) do
    if layer.isVisible and should_export_layer(layer.name) then
      local cel = layer:cel(frame)
      if cel and cel.image then
        image:drawImage(cel.image, cel.position)
      end
    end
  end
  return image
end

local config = read_json(configPath)
local sprite = app.open(config.output_aseprite)
app.activeSprite = sprite

if not app.fs.isDirectory(outDir) then
  app.fs.makeDirectory(outDir)
end

local exportMap = {
  frame_a = "dialogue_frame_a_aksil_active.png",
  frame_b = "dialogue_frame_b_protagonist_active.png",
  frame_c = "dialogue_frame_c_replacement.png",
}

for frameIndex, frameSpec in ipairs(config.frames) do
  local fileName = exportMap[frameSpec.key]
  if fileName then
    local frame = sprite.frames[frameIndex]
    local image = flatten_frame(sprite, frame)
    local outPath = app.fs.joinPath(outDir, fileName)
    image:saveAs(outPath)
    print("Exported round-trip frame: " .. outPath)
  end
end
