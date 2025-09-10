-- script.lua
local client = nil
local peer = nil
local connection_status = "Not connected"

function init()
    enet.initialize()
    client = enet.host_create(nil, 1, 2, 0, 0)
    if not client then
        connection_status = "Failed to create client"
        return
    end
    peer = enet.host_connect(client, {host = "127.0.0.1", port = 6789}, 2)
    if not peer then
        connection_status = "Failed to connect"
        return
    end
    connection_status = "Connecting..."
end

function draw()
    imgui.Begin("Network Demo", nil, {imgui.WindowFlags.NoResize, imgui.WindowFlags.NoMove})
    imgui.Text("Connection Status: " .. connection_status)
    imgui.Text("3D Cube should rotate below!")
    imgui.SliderFloat("Rotation", 0, 0, 360, "%.0f degrees")
    imgui.End()

    if imgui.Button("connect?") then
        init()
    end

    -- Poll for network events
    if client then
        local event = enet.host_service(client, 0) -- Non-blocking
        if type(event) == "table" then
            if event.type == enet.EVENT_TYPE_CONNECT then
                connection_status = "Connected!"
            elseif event.type == enet.EVENT_TYPE_DISCONNECT then
                connection_status = "Disconnected"
                peer = nil
            elseif event.type == enet.EVENT_TYPE_RECEIVE then
                print("Received: " .. enet.packet_data(event.packet))
                enet.packet_destroy(event.packet)
            end
        end
    end
end

function cleanup()
    if peer then
        enet.peer_disconnect(peer, 0)
    end
    if client then
        enet.host_destroy(client)
    end
    enet.deinitialize()
    imgui.cleanup()
end