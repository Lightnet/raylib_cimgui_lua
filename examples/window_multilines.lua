-- window 

local radio_value = 1

function draw()

    imgui.Begin("Test")
    imgui.TextColored({1.0, 0.5, 0.0, 1.0}, "InputTextMultiline")
    local edited, text = imgui.InputTextMultiline("Notes", 1024, 0, 100)
    if edited then print("Text:", text) end


    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end