# ============================================================
#  Overhead measurement campaign (experiments E1 + E2).
#
#  Protocol (docs/experiments.md):
#    1. Build the compiler in all 4 ablation configs at -O2,
#       copying each binary to experiments\bin\ (outside build\,
#       which `make clean` wipes between configs).
#    2. Generate the deterministic corpus (4 sizes).
#    3. E1 timing: per (config, size): 3 warmups, then N runs of
#       `compiler_<cfg>.exe <src> --json` with output discarded,
#       Stopwatch-timed via the call operator (&) — lower overhead
#       and less jitter than Start-Process.
#    4. E2 memory: per (config, size): M runs via Start-Process so
#       PeakWorkingSet64 is readable after exit.
#    5. Raw rows to experiments\out\timing.csv / memory.csv —
#       analysis (medians, MAD, bootstrap CIs) lives in
#       tools\analyze.py, kept separate so the raw data is a
#       reusable artifact.
#
#  Usage:  powershell -ExecutionPolicy Bypass -File tools\measure.ps1
#          [-Runs 30] [-WarmUps 3] [-MemRuns 5]
#          [-Configs full,notrace] [-SkipBuild] [-SkipCorpus] [-Append]
#
#  The full 4-config campaign exceeds a 10-minute window, so it can
#  be run in per-config chunks: first chunk without -Append (fresh
#  CSVs), later chunks with -SkipBuild -SkipCorpus -Append. Rows are
#  identical either way; chunking only changes scheduling.
# ============================================================
param(
    [int]$Runs    = 30,
    [int]$WarmUps = 3,
    [int]$MemRuns = 5,
    [string[]]$Configs = @("full", "notrace", "noprov", "baseline"),
    [switch]$SkipBuild,
    [switch]$SkipCorpus,
    [switch]$Append
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# powershell -File binds "a,b,c" to [string[]] as ONE element (comma
# splitting is PS-native syntax, not -File argument syntax) — the
# first multi-config run built a single pseudo-config literally named
# "full,notrace,noprov,baseline". Split defensively so both call
# styles work.
$configs = @($Configs | ForEach-Object { $_ -split "," } | Where-Object { $_ })
$sizes   = @(100, 1000, 5000, 20000)

New-Item -ItemType Directory -Force experiments\bin    | Out-Null
New-Item -ItemType Directory -Force experiments\corpus | Out-Null
New-Item -ItemType Directory -Force experiments\out    | Out-Null

if (-not $Append) {
    Remove-Item experiments\out\timing.csv -ErrorAction SilentlyContinue
    Remove-Item experiments\out\memory.csv -ErrorAction SilentlyContinue
}

# ── 1. Build each config at -O2 ──────────────────────────────
if (-not $SkipBuild) {
    foreach ($cfg in $configs) {
        Write-Output "=== building CONFIG=$cfg OPT=-O2 ==="
        mingw32-make clean | Out-Null
        mingw32-make CONFIG=$cfg OPT=-O2 compiler | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "build failed for CONFIG=$cfg" }
        Copy-Item build\compiler.exe "experiments\bin\compiler_$cfg.exe" -Force
    }
}

# ── 2. Corpus ────────────────────────────────────────────────
if (-not $SkipCorpus) {
    foreach ($size in $sizes) {
        python tools\gen_corpus.py --lines $size --out "experiments\corpus\p$size.cpp"
        if ($LASTEXITCODE -ne 0) { throw "corpus generation failed for size $size" }
    }
}

# ── 3. E1: timing (INTERLEAVED) ──────────────────────────────
# --check, not --json: check mode runs the full pipeline through
# assembly text but prints NOTHING on success, so the Stopwatch
# measures the compiler. Timing --json pushed megabytes of
# serialized state through PowerShell's pipeline and produced
# incoherent numbers — that dataset was discarded.
#
# Configs are interleaved ROUND-ROBIN within each run index (with
# the cycle order rotated per run to cancel position bias), not
# measured as sequential per-config blocks. The blocked protocol
# was fine while mechanism costs dwarfed environmental drift, but
# after the lazy-recorder optimisation the drift DOMINATED: one
# blocked campaign showed medians decreasing monotonically in run
# order across configs (the machine simply got faster as the
# campaign progressed), flattering whichever config ran last.
# Interleaving spreads drift evenly across all configs.
$timing = New-Object System.Collections.Generic.List[object]
foreach ($size in $sizes) {
    $src = "experiments\corpus\p$size.cpp"
    Write-Output "timing (interleaved): $size lines"
    foreach ($cfg in $configs) {
        $exe = "experiments\bin\compiler_$cfg.exe"
        for ($w = 0; $w -lt $WarmUps; $w++) { & $exe $src --check }
        if ($LASTEXITCODE -ne 0) { throw "compile failed: $cfg $size" }
    }
    for ($r = 1; $r -le $Runs; $r++) {
        $rot = $r % $configs.Count
        $order = $configs[$rot..($configs.Count-1)] + $configs[0..($rot-1)]
        if ($rot -eq 0) { $order = $configs }
        foreach ($cfg in $order) {
            $exe = "experiments\bin\compiler_$cfg.exe"
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            & $exe $src --check
            $sw.Stop()
            if ($LASTEXITCODE -ne 0) { throw "compile failed: $cfg $size run $r" }
            $timing.Add([pscustomobject]@{
                config = $cfg; size = $size; run = $r
                ms     = [math]::Round($sw.Elapsed.TotalMilliseconds, 3)
            })
        }
    }
}
$timing | Export-Csv experiments\out\timing.csv -NoTypeInformation -Append

# ── 4. E2: peak working set ──────────────────────────────────
$memory = New-Object System.Collections.Generic.List[object]
foreach ($cfg in $configs) {
    $exe = Join-Path $root "experiments\bin\compiler_$cfg.exe"
    foreach ($size in $sizes) {
        $src = Join-Path $root "experiments\corpus\p$size.cpp"
        Write-Output "memory: $cfg / $size lines"
        for ($r = 1; $r -le $MemRuns; $r++) {
            $p = Start-Process -FilePath $exe -ArgumentList "`"$src`" --check" `
                    -PassThru -NoNewWindow
            # Cache the handle BEFORE WaitForExit: without it, .NET may
            # never open the process handle and ExitCode reads $null —
            # and ($null -ne 0) is $true, so a SUCCESSFUL compile threw
            # "compile failed". Verified both ways on this machine.
            $null = $p.Handle
            # PeakWorkingSet64 is NOT readable after exit (all 80 rows
            # of the first campaign came back empty) — it must be
            # sampled while the process lives. The counter is
            # monotonic, so the last successful read is the peak up to
            # that moment; a tight refresh loop loses at most the last
            # few hundred microseconds of growth.
            $peak = 0
            while (-not $p.HasExited) {
                try {
                    $v = $p.PeakWorkingSet64
                    if ($v -gt $peak) { $peak = $v }
                } catch {}
                $p.Refresh()
            }
            $p.WaitForExit()
            if ($p.ExitCode -ne 0) { throw "compile failed (mem): $cfg $size exit=$($p.ExitCode)" }
            if ($peak -eq 0) { throw "no memory sample captured: $cfg $size run $r (process too short?)" }
            $memory.Add([pscustomobject]@{
                config    = $cfg; size = $size; run = $r
                peakBytes = $peak
            })
        }
    }
}
$memory | Export-Csv experiments\out\memory.csv -NoTypeInformation -Append

# Leave no ablated binary behind: the last config built above stays
# in build\compiler.exe otherwise (it was the BASELINE binary once,
# and a probe against it silently showed fallback traces — nothing
# was wrong except the binary being an experiment artifact).
mingw32-make clean | Out-Null

Write-Output "done ($($configs -join ',')): experiments\out\timing.csv, memory.csv"
Write-Output "after the last chunk: python tools\analyze.py"
Write-Output "note: build\ was cleaned; rebuild with mingw32-make for dev use"
