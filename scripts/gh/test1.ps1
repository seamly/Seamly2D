$Before = [datetime]"2026-09-12T00:00:00Z"

$Releases = gh release list --limit 1000 --json tagName,createdAt | ConvertFrom-Json

$Releases | ForEach-Object {
    if ([datetime]$_.createdAt -lt $Before) {
        Write-Host "$($_.createdAt)  $($_.tagName)"
    }
}