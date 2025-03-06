#!/bin/bash

# Find the example_program binary
if [ ! -f "example_program" ]; then
    echo "Error: example_program not found!"
    echo "Please run run_example.sh first."
    exit 1
fi

PATCH_FILE="/tmp/floodgate.patch"

# Create or clear patch file
if [ "$1" = "clear" ]; then
    echo "# Floodgate patch file - functions listed here will be skipped" > $PATCH_FILE
    echo "# Add function addresses one per line (in hex format)" >> $PATCH_FILE
    echo "Patch file cleared: $PATCH_FILE"
    exit 0
fi

# Show addresses of functions in the example program
if [ "$1" = "list" ] || [ -z "$1" ]; then
    echo "Function addresses in example_program:"
    echo "---------------------------------"
    nm example_program | grep " T " | sort
    echo ""
    echo "To patch a function, use: $0 add <address>"
    echo "To clear all patches, use: $0 clear"
    echo "Current patch file: $PATCH_FILE"
    
    if [ -f "$PATCH_FILE" ]; then
        echo "Currently patched functions:"
        grep -v "^#" "$PATCH_FILE" | while read addr; do
            if [ -n "$addr" ]; then
                # Try to find the symbol for this address
                func=$(nm example_program | grep -i "$addr" | awk '{print $3}')
                if [ -n "$func" ]; then
                    echo " - $addr: $func"
                else
                    echo " - $addr: <unknown>"
                fi
            fi
        done
    fi
    exit 0
fi

# Add function address to patch file
if [ "$1" = "add" ]; then
    if [ -z "$2" ]; then
        echo "Error: No address specified"
        echo "Usage: $0 add <address>"
        exit 1
    fi
    
    # Format address to proper hex
    ADDR=$(echo "$2" | sed 's/^0x//')
    
    # Create file if it doesn't exist
    if [ ! -f "$PATCH_FILE" ]; then
        echo "# Floodgate patch file - functions listed here will be skipped" > $PATCH_FILE
        echo "# Add function addresses one per line (in hex format)" >> $PATCH_FILE
    fi
    
    # Check if address is already in file
    if grep -q "$ADDR" "$PATCH_FILE"; then
        echo "Address $ADDR is already in patch file"
    else
        # Find function name if possible
        func=$(nm example_program | grep -i "$ADDR" | awk '{print $3}')
        echo "$ADDR" >> $PATCH_FILE
        if [ -n "$func" ]; then
            echo "Added $ADDR ($func) to patch file"
        else
            echo "Added $ADDR to patch file"
        fi
    fi
    exit 0
fi

# Remove function address from patch file
if [ "$1" = "remove" ]; then
    if [ -z "$2" ]; then
        echo "Error: No address specified"
        echo "Usage: $0 remove <address>"
        exit 1
    fi
    
    # Format address to proper hex
    ADDR=$(echo "$2" | sed 's/^0x//')
    
    if [ ! -f "$PATCH_FILE" ]; then
        echo "Patch file does not exist"
        exit 1
    fi
    
    # Remove the address from the file
    sed -i "/^$ADDR$/d" "$PATCH_FILE"
    echo "Removed $ADDR from patch file (if it existed)"
    exit 0
fi

# Unknown command
echo "Unknown command: $1"
echo "Usage:"
echo "  $0 list            - List all function addresses"
echo "  $0 add <address>   - Add function to patch file"
echo "  $0 remove <address> - Remove function from patch file"
echo "  $0 clear           - Clear patch file"
exit 1