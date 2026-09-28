-- Dump skills_menu.aseprite layers and export each layer alone.
local src = app.params["src"]
local outDir = app.params["outDir"]
if not src or src == "" then
  -- fallback: join cwd
  src = app.fs.joinPath(app.fs.currentPath, "assets_src/ui/skills/skills_menu.aseprite")
end
if not outDir or outDir == "" then
  outDir = app.fs.joinPath(app.fs.currentPath, "assets_src/ui/skills/export")
end

local sprite = app.open(src)
if not sprite then
  error("Could not open " .. tostring(src))
end
app.activeSprite = sprite
print("SIZE=" .. sprite.width .. "x" .. sprite.height)
print("LAYERS_BEGIN")
for i, layer in ipairs(sprite.layers) do
  print(string.format("%d|%s|vis=%s|opacity=%s", i, layer.name, tostring(layer.isVisible), tostring(layer.opacity)))
end
print("LAYERS_END")

-- Export flattened
sprite:saveCopyAs(app.fs.joinPath(outDir, "skills_menu_flat_lua.png"))

-- Export each layer solo: hide all, show one, save
local visibility = {}
for i, layer in ipairs(sprite.layers) do
  visibility[i] = layer.isVisible
end
for i, layer in ipairs(sprite.layers) do
  for j, other in ipairs(sprite.layers) do
    other.isVisible = (j == i)
  end
  local safe = layer.name:gsub("[^%w%-_]", "_")
  sprite:saveCopyAs(app.fs.joinPath(outDir, "layer_" .. string.format("%02d", i) .. "_" .. safe .. ".png"))
end
-- restore
for i, layer in ipairs(sprite.layers) do
  layer.isVisible = visibility[i]
end
print("EXPORT_DONE")
