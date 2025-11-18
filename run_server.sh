#!/bin/bash

# Configuration
PORT=55001
SHARED_DIR="./shared"
BUILD_PATH="./build/src/server/safeshare-server"

# 1. Ensure build exists
if [ ! -f "$BUILD_PATH" ]; then
    echo "Error: Server binary not found at $BUILD_PATH"
    echo "Did you run 'cmake .. && make' inside the build folder?"
    exit 1
fi

# 2. Create shared folder if missing
if [ ! -d "$SHARED_DIR" ]; then
    echo "Creating shared directory at $SHARED_DIR..."
    mkdir -p "$SHARED_DIR"
    # Create a dummy file so you have something to test with
    echo "Welcome to SafeShare" > "$SHARED_DIR/welcome.txt"
fi

# 3. Run the server
echo "Starting SafeShare Server on port $PORT..."
echo "Sharing folder: $SHARED_DIR"
echo "----------------------------------------"
"$BUILD_PATH" --port "$PORT" --shared "$SHARED_DIR"