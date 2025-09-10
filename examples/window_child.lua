-- window
--                    text,    x, y, bool for 0 or 1
-- imgui.BeginChild("ChildArea", 300, 200, 1)

function draw()
    imgui.Begin("Lua UI")

    if imgui.BeginChild("ChildArea", 300, 200, 1) then
        imgui.Text("Inside Child Window")
        
    end
    imgui.EndChild()

    imgui.End()
end
