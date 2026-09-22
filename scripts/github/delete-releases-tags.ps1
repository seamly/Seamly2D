$Before = [datetime]"2026-09-12T00:00:00Z"

$Runs = gh run list --limit 1000 --json databaseId,createdAt,workflowName |
    ConvertFrom-Json

$Runs | ForEach-Object {
    if ([datetime]$_.createdAt -lt $Before) {
        Write-Host "Deleting $($_.createdAt)  $($_.workflowName)  Run ID: $($_.databaseId)"
        gh run delete $_.databaseId
    }
}