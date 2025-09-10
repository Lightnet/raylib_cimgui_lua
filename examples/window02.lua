-- window options empty


local window_open = true

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open, {}) then
        imgui.Text("Hello from Lua!")
    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end