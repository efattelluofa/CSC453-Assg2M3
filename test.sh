#!/bin/bash

# Define the output file
output_file="testOut"

# Clear or create the output file
>"$output_file"

# Loop through each file in the tests directory
for file in tests/*; do
  # Check if it's a file
  if [[ -f "$file" ]]; then
    # Print the file name to the output file
    echo "Processing file: $file" >>"$output_file"
    # Run the executable 'compile' with the current file as input,
    # and append the output to testOut.
    ./compile <"$file" &>>"$output_file"
  fi
done
