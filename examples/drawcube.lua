-- window



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
local test_rotation = 0.0

local is_visible = true

function render()
    local time = rl.GetTime()
    -- if not imgui.IsItemActive() then
    --     -- rotation = math.fmod(time * 30.0, 360.0)
    --     rotation = math.fmod(time * 10.0, 360.0)
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

    rl.DrawCube(cubePosition)
end


function draw()
    imgui.Begin("Lua UI")

    if imgui.CollapsingHeader("Header") then
        imgui.Text("Theme Color")
    end

    local newRotation, changed = imgui.SliderFloat("Cube Y Rotation", rotation, 0.0, 360.0, "%.0f degrees")
    if changed then
        rotation = newRotation
        print(newRotation)
    end

    if imgui.Button("cube show") then
        is_visible = not is_visible
    end

    imgui.End()
    -- print(rotation)
    if is_visible then
        render()
    end
end
