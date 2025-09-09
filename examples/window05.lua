-- window options

local window_open = true

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open, {
        -- imgui.WindowFlags.NoMove,
        -- imgui.WindowFlags.NoBackground,
        -- imgui.WindowFlags.NoTitleBar

    }) then
        imgui.Text("Hello from Lua!")
    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end