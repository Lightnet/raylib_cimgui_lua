-- test....

function draw()

end

function network_update()
    print("test")
    if host then
        local event = enet.host_service(host, 0)
        if type(event) == "table" then
            if event.type == enet.EVENT_TYPE_CONNECT then
                print("Peer connected")
            elseif event.type == enet.EVENT_TYPE_RECEIVE then
                print("Received: " .. enet.packet_data(event.packet))
                enet.packet_destroy(event.packet)
            elseif event.type == enet.EVENT_TYPE_DISCONNECT then
                print("Peer disconnected")
            end
        end
    end
end