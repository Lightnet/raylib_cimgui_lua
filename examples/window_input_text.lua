-- window 

local input_text = "Enter text"

function draw()

    imgui.Begin("Test Tabs")
    imgui.Text("Button:")
    imgui.SameLine()
    if imgui.Button("Click Me") then
        print("Button clicked")
    end
    if imgui.IsItemHovered() then
        imgui.BeginTooltip()
        imgui.Text("Button is hovered")
        imgui.EndTooltip()
    end
    if imgui.IsItemActive() then
        print("Button is active")
    end
    if imgui.IsItemClicked() then
        print("Button was clicked (IsItemClicked)")
    end

    imgui.Separator()
    imgui.Spacing()

    imgui.Text("Input:")
    local edited, text = imgui.InputText("Name", 256, 0, input_text)
    if edited then
        print("Input text changed to:", text)
        input_text = text
    end

    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end