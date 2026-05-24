param(
	[switch]$optimize,
	[switch]$debuginfo,
	[switch]$buildEngine,
	[switch]$win32,
	[switch]$web
)

$engineSrc = "TileEngine/src"
$engineInc = "TileEngine/include"

$engineObjWin = "build/tilengine"
$engineLibWin = "build/libTilengine.a"
$engineObjWeb = "build/tilengine_wasm"
$engineLibWeb = "build/libTilengine_wasm.a"

$engineDefs = @(
	"-std=c99",
	"-DTLN_STATIC",
	"-DTLN_EXCLUDE_ZLIB",
	"-I$engineInc",
	"-Wno-unused-result"
)

if ($debuginfo) {
	$engineDefs += "-g"
}
if ($optimize) {
	$engineDefs += "-O2"
}

$engineSources = Get-ChildItem -Path $engineSrc -Filter *.c |
	Where-Object { $_.Name -ne "Test.c" }

function Build-EngineLib($compiler, $archiver, $objDir, $lib, $flags) {
	New-Item -ItemType Directory -Force -Path $objDir | Out-Null

	$objects = @()
	foreach ($src in $engineSources) {
		$obj = Join-Path $objDir ($src.BaseName + ".o")
		& $compiler @flags -c $src.FullName -o $obj
		if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
		$objects += $obj
	}

	& $archiver rcs $lib @objects
	if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($win32) {
	if ($buildEngine) {
		$ecf = $engineDefs + @("-Wno-format-truncation", "-D_CRT_SECURE_NO_WARNINGS")
		Build-EngineLib "clang" "llvm-ar" $engineObjWin $engineLibWin $ecf
	}
	elseif (-not (Test-Path $engineLibWin)) {
		Write-Error "$engineLibWin not found. Run './compile.ps1 -win32 -buildEngine' first."
		exit 1
	}

	$cf = @(
		"-std=c11",
		"-Wall",
		"-Wextra",
		"-Wno-unused-parameter",
		"-pedantic-errors",
		"-I$engineInc",
		"-DTLN_STATIC"
	)

	if ($optimize) {
		$cf += "-O2"
	}
	if ($debuginfo) {
		$cf += "-g"
	}

	$lf = @(
		"-lopengl32",
		"-lgdi32",
		"-luser32"
	)

	clang @cf platform_win32.c game.c $engineLibWin @lf -o build/PoolBomb.exe
	if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

	Copy-Item -Path assets -Destination build/assets -Recurse -Force
}

if ($web) {
	if ($buildEngine) {
		Build-EngineLib "emcc" "emar" $engineObjWeb $engineLibWeb $engineDefs
	}
	elseif (-not (Test-Path $engineLibWeb)) {
		Write-Error "$engineLibWeb not found. Run './compile.ps1 -web -buildEngine' first."
		exit 1
	}

	$cf = @(
		"-std=c11",
		"-Wall",
		"-Wextra",
		"-Wno-unused-parameter",
		"-I$engineInc",
		"-DTLN_STATIC"
	)

	if ($optimize) {
		$cf += "-O2"
	}
	if ($debuginfo) {
		$cf += "-g"
	}

	emcc @cf platform_emscripten.c game.c $engineLibWeb --shell-file shell.html --preload-file assets -o build/PoolBomb.html
	if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
