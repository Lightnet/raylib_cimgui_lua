-- ref test data
-- local imgui = require("cimgui")

-- Global variables (mirroring C code)
local screenWidth = 800
local screenHeight = 450
local camera = {
    position = { x = 5.0, y = 5.0, z = 5.0 },
    target = { x = 0.0, y = 0.0, z = 0.0 },
    up = { x = 0.0, y = 1.0, z = 0.0 },
    fovy = 45.0
}
local cubePosition = { x = 0.0, y = 0.0, z = 0.0 }
local rotation = 0.0

-- Cube data (translated from drawcube.c)
local RL_TRIANGLES = 4 -- RL_TRIANGLES mode from rlgl.h
local cubeVertices = {
    { x = -1.0, y = -1.0, z =  1.0 }, -- 0
    { x =  1.0, y = -1.0, z =  1.0 }, -- 1
    { x =  1.0, y =  1.0, z =  1.0 }, -- 2
    { x = -1.0, y =  1.0, z =  1.0 }, -- 3
    { x = -1.0, y = -1.0, z = -1.0 }, -- 4
    { x = -1.0, y =  1.0, z = -1.0 }, -- 5
    { x =  1.0, y =  1.0, z = -1.0 }, -- 6
    { x =  1.0, y = -1.0, z = -1.0 }, -- 7
    { x = -1.0, y =  1.0, z = -1.0 }, -- 8
    { x = -1.0, y =  1.0, z =  1.0 }, -- 9
    { x =  1.0, y =  1.0, z =  1.0 }, -- 10
    { x =  1.0, y =  1.0, z = -1.0 }, -- 11
    { x = -1.0, y = -1.0, z = -1.0 }, -- 12
    { x =  1.0, y = -1.0, z = -1.0 }, -- 13
    { x =  1.0, y = -1.0, z =  1.0 }, -- 14
    { x = -1.0, y = -1.0, z =  1.0 }, -- 15
    { x =  1.0, y = -1.0, z = -1.0 }, -- 16
    { x =  1.0, y =  1.0, z = -1.0 }, -- 17
    { x =  1.0, y =  1.0, z =  1.0 }, -- 18
    { x =  1.0, y = -1.0, z =  1.0 }, -- 19
    { x = -1.0, y = -1.0, z = -1.0 }, -- 20
    { x = -1.0, y = -1.0, z =  1.0 }, -- 21
    { x = -1.0, y =  1.0, z =  1.0 }, -- 22
    { x = -1.0, y =  1.0, z = -1.0 }  -- 23
}
local indices = {
    0, 1, 2, 2, 3, 0,   -- Front
    4, 5, 6, 6, 7, 4,   -- Back
    8, 9, 10, 10, 11, 8, -- Top
    12, 13, 14, 14, 15, 12, -- Bottom
    16, 17, 18, 18, 19, 16, -- Right
    20, 21, 22, 22, 23, 20  -- Left
}
local colors = {
    { r = 255, g = 0,   b = 0,   a = 255 }, -- Front: Red
    { r = 0,   g = 255, b = 0,   a = 255 }, -- Back: Green
    { r = 0,   g = 0,   b = 255, a = 255 }, -- Top: Blue
    { r = 255, g = 255, b = 0,   a = 255 }, -- Bottom: Yellow
    { r = 255, g = 0,   b = 255, a = 255 }, -- Right: Magenta
    { r = 0,   g = 255, b = 255, a = 255 }  -- Left: Cyan
}

-- DrawCube function translated from drawcube.c
function DrawCube(position)
    rl.rlTranslatef(position.x, position.y, position.z)
    
    rl.rlBegin(RL_TRIANGLES)
    for face = 0, 5 do
        local color = colors[face + 1]
        rl.rlColor4ub(color.r, color.g, color.b, color.a)
        for i = 0, 5 do
            local idx = face * 6 + i + 1 -- Lua indices are 1-based
            local v = cubeVertices[indices[idx] + 1]
            rl.rlVertex3f(v.x, v.y, v.z)
        end
    end
    rl.rlEnd()
end

-- Render function
function render()
    local time = rl.GetTime()
    -- if not imgui.IsItemActive() then
    --     rotation = math.fmod(time * 30.0, 360.0)
    -- end

    local aspect = screenWidth / screenHeight
    local proj = rl.MatrixPerspective(camera.fovy * (math.pi / 180.0), aspect, 0.1, 1000.0)
    rl.rlSetMatrixProjection(proj)

    local view = rl.MatrixLookAt(camera.position, camera.target, camera.up)
    local rot = rl.MatrixRotateY(rotation * (math.pi / 180.0))
    local trans = rl.MatrixTranslate(cubePosition.x, cubePosition.y, cubePosition.z)
    local model = rl.MatrixMultiply(rot, trans)
    local modelView = rl.MatrixMultiply(model, view)
    rl.rlSetMatrixModelview(modelView)

    DrawCube(cubePosition) -- Call Lua DrawCube
end

-- Draw function for ImGui
function draw()
    -- imgui.NewFrame()
    
    imgui.Begin("Lua-Controlled Cube", nil, 0)
    imgui.Text("This is the Lua-controlled UI!")
    imgui.Text(string.format("Current Rotation: %.1f degrees", rotation))
    local newRotation, changed = imgui.SliderFloat("Cube Y Rotation", rotation, 0.0, 360.0, "%.0f degrees")
    if changed then
        rotation = newRotation
        print(newRotation)
    end
    imgui.End()
    
    render()
    
    -- imgui.Render()
end

function cleanup()
    print("Lua cleanup called")
end