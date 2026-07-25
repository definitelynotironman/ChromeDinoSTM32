param(
    [ValidateSet('build', 'test', 'flash', 'sim')][string]$Action = 'build',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$GccPath = ''
)
# Run from the project directory even when invoked from somewhere else.
Push-Location $PSScriptRoot
try {
    $arguments = @('tools/build.py', $Action, '--config', $Configuration)
    if ($GccPath) { $arguments += @('--gcc-path', $GccPath) }
    & python @arguments
    if ($LASTEXITCODE -ne 0) { throw "Build action failed with exit code $LASTEXITCODE" }
} finally {
    Pop-Location
}
