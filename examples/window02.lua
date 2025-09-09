-- window options

local window_open = true

function draw()
    if imgui.begin_window("Lua ImGui Demo", window_open, 0) then
        imgui.text("Hello from Lua!")
    end
    imgui.end_window()
end

function cleanup()
    print("Lua cleanup called")
end