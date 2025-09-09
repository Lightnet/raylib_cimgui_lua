function draw()
    imgui.begin_window("Lua UI")
    imgui.text("Hello from Lua!")
    imgui.end_window()
end

function cleanup()
    print("Lua cleanup called")
end