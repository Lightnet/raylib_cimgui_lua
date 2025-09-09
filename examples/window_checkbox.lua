-- window options

local window_open = true
local slider_value = 0.5


function draw()
    if imgui.Begin("Lua ImGui Demo", window_open, {}) then
        imgui.Text("Hello from Lua!")
        -- Checkbox example: Pass value, receive updated value and changed flag
        local new_checkbox, changed2 = imgui.CheckBox("Enable Feature", checkbox_value)
        if changed2 then
            checkbox_value = new_checkbox  -- Update local variable
            print("Checkbox changed to: " .. tostring(checkbox_value))
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end