{
	"targets": [{
		"target_name": "xbps",
		"sources": [
			"src/xbps.cpp",
		],
		"include_dirs" : [
			"<!(node -e \"require('nan')\")"
		],
		"cflags": [
			"-g <!@(pkg-config --cflags libxbps)"
		],
		"libraries": [
			"<!@(pkg-config --libs libxbps)"
		]
	}]
}
