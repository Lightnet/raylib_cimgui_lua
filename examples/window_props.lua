-- window
--                    text,    x, y, bool for 0 or 1
-- imgui.BeginChild("ChildArea", 300, 200, 1)
function draw()
    imgui.Begin("Lua UI")

    local pos_x, pos_y = imgui.GetWindowPos()
    local size_x, size_y = imgui.GetWindowSize()
    imgui.TextColored({1.0, 1.0, 0.0, 1.0}, string.format("Window Pos: %.1f, %.1f", pos_x, pos_y))
    imgui.TextColored({1.0, 1.0, 0.0, 1.0}, string.format("Window Size: %.1f, %.1f", size_x, size_y))
    imgui.TextColored({1.0, 1.0, 0.0, 1.0}, string.format("Window Width: %.1f", imgui.GetWindowWidth()))
    imgui.TextColored({1.0, 1.0, 0.0, 1.0}, string.format("Window Height: %.1f", imgui.GetWindowHeight()))
    imgui.TextColored({0.0, 1.0, 1.0, 1.0}, string.format("Appearing: %s", imgui.IsWindowAppearing()))
    imgui.TextColored({0.0, 1.0, 1.0, 1.0}, string.format("Collapsed: %s", imgui.IsWindowCollapsed()))
    imgui.TextColored({0.0, 1.0, 1.0, 1.0}, string.format("Focused: %s", imgui.IsWindowFocused()))
    imgui.TextColored({0.0, 1.0, 1.0, 1.0}, string.format("Hovered: %s", imgui.IsWindowHovered()))

    imgui.End()
end
