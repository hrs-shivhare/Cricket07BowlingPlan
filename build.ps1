param([string]$Zig, [string]$GameDirectory = 'E:\Cricket07')
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
if (-not $Zig) { $Zig = Join-Path $projectRoot '..\..\work\toolchains\zig-x86_64-windows-0.15.2\zig.exe' }
$Zig = (Resolve-Path -LiteralPath $Zig).Path
$dist = Join-Path $projectRoot 'dist'
$testDir = Join-Path $projectRoot 'tests\bin'
New-Item -ItemType Directory -Force -Path $dist,$testDir | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $projectRoot '..\..\work\toolchains\zig-cache'
$src = Join-Path $projectRoot 'src'
$plugin = Join-Path $dist 'Cricket07BowlingPlan.asi'
$exe = Join-Path $GameDirectory 'Cricket07.exe'
$expected = 'D776DBA0CC506B968F7D228E036CF42AEB4ABE45A8D1AD0D7226CDEBF91553A2'
$initialExeHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
if ($initialExeHash -ne $expected) {
    Write-Warning 'Game SHA-256 differs from reference; continuing with PE/layout and hook-signature validation.'
}
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror -shared `
    (Join-Path $src 'plugin.c') (Join-Path $src 'schedule.c') (Join-Path $src 'long_schedule.c') `
    (Join-Path $src 'field_policy.c') (Join-Path $src 'hook.c') `
    (Join-Path $src 'exports.def') -ladvapi32 -lkernel32 -o $plugin
if ($LASTEXITCODE) { throw 'ASI compilation failed.' }
$scheduler = Join-Path $testDir 'schedule_test.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\schedule_test.c') (Join-Path $src 'schedule.c') `
    (Join-Path $src 'long_schedule.c') -o $scheduler
if ($LASTEXITCODE) { throw 'Schedule test compilation failed.' }
& $scheduler
if ($LASTEXITCODE) { throw 'Schedule tests failed.' }
$longScheduler = Join-Path $testDir 'long_schedule_test.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\long_schedule_test.c') (Join-Path $src 'long_schedule.c') -o $longScheduler
if ($LASTEXITCODE) { throw 'Long schedule test compilation failed.' }
& $longScheduler
if ($LASTEXITCODE) { throw 'Long schedule tests failed.' }
$native = Join-Path $testDir 'native_test.exe'
& $Zig cc -target x86-windows-gnu -std=c11 -O2 -Wall -Wextra -Werror `
    (Join-Path $projectRoot 'tests\native_test.c') (Join-Path $src 'schedule.c') `
    (Join-Path $src 'long_schedule.c') (Join-Path $src 'field_policy.c') `
    (Join-Path $src 'hook.c') -ladvapi32 -lkernel32 -o $native
if ($LASTEXITCODE) { throw 'Native test compilation failed.' }
& $native $plugin $exe
if ($LASTEXITCODE) { throw 'Native integration tests failed.' }
if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ne $initialExeHash) {
    throw 'Executable changed during validation.'
}
Get-FileHash -LiteralPath $plugin -Algorithm SHA256
