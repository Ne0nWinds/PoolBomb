param(
	[switch]$optimize,
	[switch]$debuginfo,
	[switch]$win32,
	[switch]$web
)


if ($win32) {
	$cf = @(
		"-std=c99",
		"-Wall",
		"-Wextra",
		"-Wno-unused-parameter",
		"-pedantic-errors",
		"-Ilib/",
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
	$cf = @(
		"-std=c99",
		"-Wall",
		"-Wextra",
		"-Wno-unused-parameter",
		"-msimd128",
		"-Ilib/",
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
