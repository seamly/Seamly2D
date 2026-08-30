gh release list --limit 1000 --json tagName,createdAt --jq '.[] | select(.createdAt < "2026-06-01T00:00:00Z") | [.tagName, .createdAt] | @tsv' |
ForEach-Object {
    gh release delete $_ --cleanup-tag --yes
}

