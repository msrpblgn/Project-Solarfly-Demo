-- Build dialogue_behavior_mockup.aseprite from mockup_config.json (Milestone 1).

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
local root = config.root:gsub("\\", "/")
local width = config.width
local height = config.height

local sprite = Sprite(width, height, ColorMode.RGB)
app.activeSprite = sprite

while #sprite.layers > 0 do
  sprite:deleteLayer(sprite.layers[1])
end

local layerByName = {}
for _, layerName in ipairs(config.layer_order_bottom_to_top) do
  local layer = sprite:newLayer()
  layer.name = layerName
  layerByName[layerName] = layer
end

local function load_image(path)
  return Image{ fromFile=path }
end

local function set_layer_image(layerName, frameNumber, relativePath)
  local layer = layerByName[layerName]
  if not layer then
    error("Missing layer: " .. layerName)
  end
  while #sprite.frames < frameNumber do
    sprite:newEmptyFrame()
  end
  local frame = sprite.frames[frameNumber]
  local imagePath = root .. "/" .. relativePath .. "/" .. layerName .. ".png"
  local image = load_image(imagePath)
  local cel = layer:cel(frame)
  if not cel then
    cel = sprite:newCel(layer, frame)
  end
  cel.image = image
end

for frameIndex, frameSpec in ipairs(config.frames) do
  local layerDir = frameSpec.layer_dir
  for _, layerName in ipairs(config.layer_order_bottom_to_top) do
    set_layer_image(layerName, frameIndex, layerDir)
  end
end

while #sprite.tags > 0 do
  sprite:deleteTag(sprite.tags[1])
end

for frameIndex, frameSpec in ipairs(config.frames) do
  local tag = sprite:newTag()
  tag.name = frameSpec.tag
  tag.fromFrame = sprite.frames[frameIndex]
  tag.toFrame = sprite.frames[frameIndex]
  tag.aniDir = AniDir.FORWARD
end

sprite:saveAs(config.output_aseprite)
print("Saved Aseprite mockup: " .. config.output_aseprite)
