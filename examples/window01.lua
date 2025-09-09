-- window

function draw()
    if imgui.Begin("Lua ImGui Demo") then
        imgui.Text("Hello from Lua!")
    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end