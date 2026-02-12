-- ============================================================
-- Headless atlas export + C header for Raylib (ROWS, 2048x2048)
-- ============================================================

local PIVOT_LAYER_NAME = "Pivot"
local ATLAS_SIZE = 2048

-- ------------------- helpers -------------------
local function ceilDiv2(x) return math.floor((x + 1) / 2) end
local function baseName(path) return app.fs.fileTitle(path) end
local function ensureDir(path)
  if not app.fs.isDirectory(path) then
    app.fs.makeAllDirectories(path)
  end
end
local function sanitizeName(name)
  local s = name:upper()
  s = s:gsub("[^%w]", "_")
  return "SPRITE_" .. s
end
local function normalizeFrameName(filename)
  local n = filename:gsub("%.aseprite$", "")
  n = n:gsub("%s+%d+$", "") -- strip frame index
  return n
end

-- ------------------- setup paths -------------------
local BASE_DIR  = app.fs.filePath("./assets/")
-- local BASE_DIR  = app.fs.filePath("~/asteroids/assets/")
local ATLAS_DIR = app.fs.joinPath(BASE_DIR, "atlas")
ensureDir(ATLAS_DIR)
-- print(BASE_DIR)
-- print(ATLAS_DIR)

local ATLAS_IMAGE = app.fs.joinPath(ATLAS_DIR, "atlas.png")
local TEMP_JSON   = app.fs.joinPath(ATLAS_DIR, "_temp.json")
local H_FILE      = app.fs.joinPath(ATLAS_DIR, "../../assetsData.h")
local SHAPE_PADDING = 1

-- ------------------- collect .aseprite files -------------------
local files = {}
for _, file in ipairs(app.fs.listFiles(BASE_DIR)) do
  -- print(file)
  if file:lower():match("%.aseprite$") then
	  -- print(file)
	  table.insert(files, app.fs.joinPath(BASE_DIR, file))
  end
end
if #files == 0 then return end

-- ------------------- gather pivot/meta info -------------------
local spriteMeta = {}

for _, path in ipairs(files) do
  local spr = Sprite{ fromFile = path }
  local name = baseName(path)

  -- print(spr.filename)
  -- print(#spr.frames)
  local frameCount = #spr.frames
  local frameW     = spr.width * frameCount
  local frameH     = spr.height

  local pivotX, pivotY = 0, 0

  for _, layer in ipairs(spr.layers) do
    if layer.name == PIVOT_LAYER_NAME then
      local cel = layer.cels[1]
      if cel then
        pivotX = cel.position.x - ceilDiv2(frameW/frameCount)
        pivotY = cel.position.y - frameH
      end
      break
    end
  end

  spriteMeta[name] = {
    frameCount  = frameCount,
    frameWidth  = frameW,
    frameHeight = frameH,
    pivotX      = pivotX,
    pivotY      = pivotY,
    offsetX     = 0,
    offsetY     = 0,
  }
  spr:close()
end

-- ------------------- pack atlas via CLI -------------------
local cliPaths = {}
for _, f in ipairs(files) do
  table.insert(cliPaths, '"' .. f .. '"')
end
local filesStr = table.concat(cliPaths, " ")

local cmd = string.format(
  'aseprite -b %s ' ..
  '--sheet "%s" ' ..
  '--data "%s" ' ..
  '--sheet-type rows ' ..
  '--sheet-width %d ' ..
  '--sheet-height %d ' ..
  '--shape-padding %d ' ..
  '--format json-array',
  filesStr,
  ATLAS_IMAGE,
  TEMP_JSON,
  ATLAS_SIZE,
  ATLAS_SIZE,
  SHAPE_PADDING
)

os.execute(cmd)

-- ------------------- parse offsets from JSON (FIXED) -------------------
local f = io.open(TEMP_JSON, "r")
local json = f:read("*a")
f:close()

for filename, x, y in json:gmatch(
  '"filename"%s*:%s*"([^"]-)".-"frame"%s*:%s*{%s*"x"%s*:%s*(%d+),%s*"y"%s*:%s*(%d+)'
) do
  local name = normalizeFrameName(filename)
  local m = spriteMeta[name]
  if m then
    m.offsetX = tonumber(x) 
    m.offsetY = tonumber(y)
  end
end

-- ------------------- write C header -------------------
local h = io.open(H_FILE, "w")

h:write("// AUTO-GENERATED — DO NOT EDIT\n")
h:write("#pragma once\n")
h:write("#include \"raylib.h\"\n")
-- h:write("#include \"assetsUtils.h\"\n")
h:write("#define internal static\n\n")

-- Sprite struct
h:write("typedef struct Sprite {\n")
h:write("    Texture2D texture;\n")
h:write("    Rectangle coords;\n")
h:write("    Vector2 pivotOffset;\n")
h:write("    int numFrames;\n")
h:write("} Sprite;\n\n")

-- enum SpriteID
h:write("typedef enum SpriteID {\n")
local spriteNames = {}
for name in pairs(spriteMeta) do
  table.insert(spriteNames, name)
end
table.sort(spriteNames)

for _, name in ipairs(spriteNames) do
  h:write("    " .. sanitizeName(name) .. ",\n")
end
h:write("    SPRITE_COUNT\n")
h:write("} SpriteID;\n\n")

-- getSprite()
h:write("static inline Sprite getSprite(SpriteID spriteID) {\n")
h:write("    Sprite s = {0};\n")
h:write("    switch(spriteID) {\n")

for _, name in ipairs(spriteNames) do
  local m = spriteMeta[name]
  h:write(string.format(
    "        case %s: { s.coords = (Rectangle){%d, %d, %d, %d}; s.pivotOffset = (Vector2){%d, %d}; s.numFrames = %d; break; }\n",
    sanitizeName(name),
    m.offsetX - (m.frameWidth/m.frameCount)*(m.frameCount-1),
    m.offsetY,
    m.frameWidth,
    m.frameHeight,
    m.pivotX,
    m.pivotY,
    m.frameCount
  ))
end

h:write("        default: break;\n")
h:write("    }\n")
h:write("    return s;\n")
h:write("}\n")
h:close()

