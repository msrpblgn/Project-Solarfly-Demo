print("CWD=" .. tostring(app.fs.currentPath))
local configPath = app.fs.joinPath(app.fs.currentPath, "assets_src/ui/dialogue/build/mockup_config.json")
print("CONFIG=" .. tostring(configPath))
print("EXISTS=" .. tostring(app.fs.isFile(configPath)))
