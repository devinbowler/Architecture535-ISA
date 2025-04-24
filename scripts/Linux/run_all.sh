# Get the directory where the script is located
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

echo "Starting the compilation process..."
# Run the compilation script
bash "$SCRIPT_DIR/compile.sh"

echo "Starting the API..."
# Try to detect the available terminal
if command -v x-terminal-emulator > /dev/null 2>&1; then
    x-terminal-emulator -e "bash \"$SCRIPT_DIR/run_api.sh\"" &
elif command -v gnome-terminal > /dev/null 2>&1; then
    gnome-terminal -- bash -c "\"$SCRIPT_DIR/run_api.sh\"" &
elif command -v xterm > /dev/null 2>&1; then
    xterm -e "bash \"$SCRIPT_DIR/run_api.sh\"" &
elif command -v konsole > /dev/null 2>&1; then
    konsole -e "bash \"$SCRIPT_DIR/run_api.sh\"" &
elif command -v terminator > /dev/null 2>&1; then
    terminator -e "bash \"$SCRIPT_DIR/run_api.sh\"" &
elif command -v xfce4-terminal > /dev/null 2>&1; then
    xfce4-terminal -e "bash \"$SCRIPT_DIR/run_api.sh\"" &
else
    # If no graphical terminal is found, use screen or tmux if available
    if command -v screen > /dev/null 2>&1; then
        screen -dmS api_server bash -c "\"$SCRIPT_DIR/run_api.sh\""
        echo "Started API in a screen session. Connect with: screen -r api_server"
    elif command -v tmux > /dev/null 2>&1; then
        tmux new-session -d -s api_server "\"$SCRIPT_DIR/run_api.sh\""
        echo "Started API in a tmux session. Connect with: tmux attach -t api_server"
    else
        echo "No suitable terminal emulator found. Running API in background."
        bash "$SCRIPT_DIR/run_api.sh" > api_output.log 2>&1 &
        echo "API running in background. Check api_output.log for output."
    fi
fi

# Wait a moment for the API to start
sleep 2

echo "Starting the interface..."
# Try to detect the available terminal
if command -v x-terminal-emulator > /dev/null 2>&1; then
    x-terminal-emulator -e "bash \"$SCRIPT_DIR/run_interface.sh\"" &
elif command -v gnome-terminal > /dev/null 2>&1; then
    gnome-terminal -- bash -c "\"$SCRIPT_DIR/run_interface.sh\"" &
elif command -v xterm > /dev/null 2>&1; then
    xterm -e "bash \"$SCRIPT_DIR/run_interface.sh\"" &
elif command -v konsole > /dev/null 2>&1; then
    konsole -e "bash \"$SCRIPT_DIR/run_interface.sh\"" &
elif command -v terminator > /dev/null 2>&1; then
    terminator -e "bash \"$SCRIPT_DIR/run_interface.sh\"" &
elif command -v xfce4-terminal > /dev/null 2>&1; then
    xfce4-terminal -e "bash \"$SCRIPT_DIR/run_interface.sh\"" &
else
    # If no graphical terminal is found, use screen or tmux if available
    if command -v screen > /dev/null 2>&1; then
        screen -dmS interface bash -c "\"$SCRIPT_DIR/run_interface.sh\""
        echo "Started interface in a screen session. Connect with: screen -r interface"
    elif command -v tmux > /dev/null 2>&1; then
        tmux new-session -d -s interface "\"$SCRIPT_DIR/run_interface.sh\""
        echo "Started interface in a tmux session. Connect with: tmux attach -t interface"
    else
        echo "No suitable terminal emulator found. Running interface in background."
        bash "$SCRIPT_DIR/run_interface.sh" > interface_output.log 2>&1 &
        echo "Interface running in background. Check interface_output.log for output."
    fi
fi

echo "All components started"