param(
	[switch]$optimize,
	[switch]$debuginfo
)

# Animation preview tool build.
#
# Reuses the exact same emscripten platform layer as the web game
# (../platform_emscripten.c); the only swap is the game logic: preview.c
# instead of ../game.c. Output goes to ../build/preview/.
#
# Run from the repo root:   .\animation_preview_tool\compile.ps1
# or from this folder:      .\compile.ps1

$ErrorActionPreference = "Stop"

# Resolve paths relative to this script so it works from any working directory.
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$out  = Join-Path $root "build/preview"

$cf = @(
	"-std=c99",
	"-Wall",
	"-Wextra",
	"-Wno-unused-parameter",
	"-msimd128",
	"-I$root/lib/",
	"-sINITIAL_MEMORY=16973824",
	"-sALLOW_MEMORY_GROWTH=1",
	"-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_SetAnimationSpeed,_SetCharacterIndex,_ReloadSpritesetFromMemory,_GetCharacterCount",
	"-sEXPORTED_RUNTIME_METHODS=HEAPU8"
)

if ($optimize) {
	$cf += "-O2"
}
if ($debuginfo) {
	$cf += "-g"
}

New-Item -ItemType Directory -Force $out | Out-Null

emcc @cf `
	"$PSScriptRoot/platform_emscripten.c" `
	"$PSScriptRoot/preview.c" `
	--shell-file "$PSScriptRoot/shell.html" `
	--preload-file "$root/assets@assets" `
	-o "$out/PreviewTool.html"

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
