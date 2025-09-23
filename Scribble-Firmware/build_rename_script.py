Import("env")
import json

# Fetch all definition build flags ('-D')
my_flags = env.ParseFlags(env['BUILD_FLAGS'])
cppdefines = my_flags.get("CPPDEFINES")

# Get the firmware version number and matching hardware version tag
for define in cppdefines:
	if define[0] == 'FW_VERSION':
		version = define[1]
		version = version.strip('\"')
		fw_version = version
	if define[0] == 'HW_TAG':
		hw_tag = define[1]
	if define[0] == 'MODEL':
		model = define[1]

# Check if handle alpha and beta version strings
if version.find('beta') != -1  or version.find('alpha') != -1:
	version = version.split('-', 1)
	version = version[0] + '.' + hw_tag + '-' + version[1]
else:
	version = version + '.' + hw_tag

version = model + '_v' + version

print('Building for version: ' + version)
env.Replace(PROGNAME=version)

def create_flash_map(extra_flash_images,
							bootloader_filename,
							partitions_filename,
							ota_filename,
							app_filename,
							app_address,
							output_path=None):
	import os
	import json
	"""
	Create a JSON flash map from EXTRA_FLASH_IMAGES using provided filenames
	
	Args:
		extra_flash_images: List of tuples from env["EXTRA_FLASH_IMAGES"]
		bootloader_filename: Name for the bootloader file
		partitions_filename: Name for the partitions file  
		ota_filename: Name for the OTA file
		output_path: Optional path to save the JSON file
	"""
	
	flash_map = {
		"bootloader": [],
		"partitions": [], 
		"ota": [],
		"application": []
	}

	# Add the application entry
	flash_map["application"].append({
	"address": app_address,
	"fileName": app_filename
	})
	
	# Iterate through the flash images and assign provided filenames
	for address, file_path in extra_flash_images:
		original_filename = os.path.basename(file_path)
		
		# Determine the type based on original filename patterns
		if "bootloader" in original_filename.lower():
			flash_type = "bootloader"
			assigned_filename = bootloader_filename
		elif "partitions" in original_filename.lower():
			flash_type = "partitions"
			assigned_filename = partitions_filename
		elif "boot_app0" in original_filename.lower() or "ota" in original_filename.lower():
			flash_type = "ota"
			assigned_filename = ota_filename
		else:
			continue
		
		# Add to the flash map with the assigned filename
		flash_map[flash_type].append({
			"address": address,
			"fileName": assigned_filename
		})
	
	# Convert to JSON
	json_output = json.dumps(flash_map, indent=2)
	
	# Save to file if output path provided
	if output_path:
		with open(output_path, 'w') as f:
			f.write(json_output)
		print(f"Flash map saved to: {output_path}")
	
	return flash_map, json_output

def print_env_variables():
	print("\n=== ENV VARIABLES ===")
	for key, value in sorted(env.Dictionary().items()):
		print(f"{key}: {value}")

def post_build_action(source, target, env):
	
	import shutil
	import os
	#print_env_variables()
	# Get the application binary and copy it to the output folder
	firmware_path = os.path.join(env['PROJECT_DIR'], 'build_outputs/' + fw_version + '/' + model)
	print(firmware_path)
	# Create a new build_outputs folder if there isn't one
	if not os.path.exists(firmware_path):
		os.makedirs(firmware_path)

	# Check if the current firmware version exists, and remove any old files
	# todo

	print(target[0].path)
	shutil.copy(target[0].path, firmware_path)

	# Get the extra images from the build environment variable
	bootloader_file_name = ('bootloader_' + version + '.bin')
	partitions_file_name = ('partitions_' + version + '.bin')
	ota_file_name = ('ota_' + version + '.bin')
	
	# Copy and rename the bootloader, partitions, and ota files
	shutil.copy(env["FLASH_EXTRA_IMAGES"][0][1], (firmware_path + '/' + bootloader_file_name))
	shutil.copy(env["FLASH_EXTRA_IMAGES"][1][1], (firmware_path + '/' + partitions_file_name))
	shutil.copy(env["FLASH_EXTRA_IMAGES"][2][1], (firmware_path + '/' + ota_file_name))

	create_flash_map(	env["FLASH_EXTRA_IMAGES"], bootloader_file_name, partitions_file_name, ota_file_name,
							env["PROGNAME"], env["ESP32_APP_OFFSET"], (firmware_path + '/flash_map_' + version + '.json'))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)