param(
	[switch]$optimize,
	[switch]$debuginfo,
	[switch]$win32,
	[switch]$web,
	[switch]$watch
)


function Invoke-Build {
	if ($win32) {
		$cf = @(
			"-std=c99",
			"-Wall",
			"-Wextra",
			"-Wno-unused-parameter",
			"-Wno-unused-function",
			"-pedantic-errors",
			"-DPLATFORM_WIN32",
			"-Ilib/"
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
			"-lxinput"
		)

		clang @cf platform_win32.c game.c $engineLibWin @lf -o build/PoolBomb.exe
		if ($LASTEXITCODE -ne 0) { return }

		rm -Recurse -Force build/assets
		$null = mkdir build/assets
		Copy-Item -Path assets/* -Destination build/assets -Recurse -Force
	}

	if ($web) {
		$cf = @(
			"-std=c99",
			"-Wall",
			"-Wextra",
			"-Wno-unused-parameter",
			"-Wno-unused-function",
			"-msimd128",
			"-mbulk-memory",
			"-Ilib/",
			"-DPLATFORM_EMSCRIPTEN",
			"-sINITIAL_MEMORY=16973824"
		)

		if ($optimize) {
			$cf += "-O2"
		}
		if ($debuginfo) {
			$cf += "-g"
		}

		emcc @cf platform_emscripten.c game.c $engineLibWeb --shell-file shell.html --preload-file assets -o build/PoolBomb.html
		if ($LASTEXITCODE -ne 0) { return }
	}
}

function Write-BuildResult {
	$stamp = Get-Date -Format "HH:mm:ss"
	if ($LASTEXITCODE -eq 0) {
		Write-Host "[$stamp] Build succeeded." -ForegroundColor Green
	} else {
		Write-Host "[$stamp] Build failed (exit $LASTEXITCODE)." -ForegroundColor Red
	}
}


if ($watch) {
	Invoke-Build
	Write-BuildResult

	$watcher = New-Object System.IO.FileSystemWatcher
	$watcher.Path = (Get-Location).Path
	$watcher.IncludeSubdirectories = $true
	$watcher.NotifyFilter = [System.IO.NotifyFilters]::LastWrite -bor [System.IO.NotifyFilters]::FileName
	$watcher.EnableRaisingEvents = $true

	Write-Host "Watching .c/.h files for changes. Press Ctrl+C to stop." -ForegroundColor Cyan
	while ($true) {
		$change = $watcher.WaitForChanged([System.IO.WatcherChangeTypes]::Changed, 500)
		if ($change.TimedOut) { continue }
		if ($change.Name -notmatch '\.(c|h)$') { continue }

		$stamp = Get-Date -Format "HH:mm:ss"
		Write-Host ""
		Write-Host "[$stamp] $($change.Name) changed, building..." -ForegroundColor Cyan
		Invoke-Build
		Write-BuildResult
	}
} else {
	Invoke-Build
	exit $LASTEXITCODE
}
