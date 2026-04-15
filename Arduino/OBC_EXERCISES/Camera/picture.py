file_path = r'C:\Users\rudolph\Downloads\datat.txt' 
# Open the text file you saved from CoolTerm
with open(file_path, 'r') as f:
    # Read the text and remove spaces/newlines so it's one long string
    hex_string = f.read().replace(' ', '').replace('\n', '').replace('\r', '')

# Convert the hex string into actual binary bytes
image_bytes = bytes.fromhex(hex_string)

# Save those bytes as a JPG file
with open(r'C:\Users\rudolph\Downloads\satellite_photo.jpg', 'wb') as f:
    f.write(image_bytes)

print("Success! Your image is saved as satellite_photo.jpg")