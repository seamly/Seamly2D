gh release list --limit 1000 --json tagName,createdAt --jq '.[] | select(.createdAt < "2026-09-12T00:00:00Z") | .tagName' |
ForEach-Object {
    gh release delete $_ --cleanup-tag --yes
}

