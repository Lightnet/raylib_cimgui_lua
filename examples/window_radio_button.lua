-- window 

local radio_value = 1

function draw()

    imgui.Begin("Test Tabs")
    imgui.Text("RadioButton:")
        local clicked, new_value = imgui.RadioButton("Option 1", radio_value, 1)
        if clicked then
            radio_value = new_value
            print("Radio selected: Option 1")
        end
        imgui.SameLine()
        clicked, new_value = imgui.RadioButton("Option 2", radio_value, 2)
        if clicked then
            radio_value = new_value
            print("Radio selected: Option 2")
        end

    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end