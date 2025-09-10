-- window 
local ref_color = {0.5, 0.5, 0.5, 1.0}

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua ColorEdit4!")
        -- ColorButton with table flags
        local button_color = {0.0, 1.0, 0.0, 1.0} -- Green
        if imgui.ColorButton("Color Button", button_color, {
            imgui.ColorEditFlags.NoTooltip,
            imgui.ColorEditFlags.NoDragDrop
        }, {40, 40}) then
            print("Color button clicked!")
        end
    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end