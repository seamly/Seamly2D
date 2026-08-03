param(
    [string]$ThreeMfPath = "output/richmond-shirt_v1_v061-02_2601281626.3mf",
    [switch]$Release
)

$ErrorActionPreference = "Stop"

Write-Host "Running 3MF validation..."
python "scripts/validate_3mf.py" $ThreeMfPath
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
