function draw()
    imgui.Begin("Lua UI")
    imgui.Text("Hello from Lua!")
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end