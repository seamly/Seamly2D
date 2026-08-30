git fetch --tags

git for-each-ref refs/tags \
  --format='%(refname:short) %(creatordate:iso8601)' |
awk '$2 < "2025-01-01" {print $1}' |
while IFS= read -r tag; do
  if git ls-remote --exit-code --tags origin "refs/tags/$tag" >/dev/null 2>&1; then
    echo "Deleting $tag"
    git push origin --delete "$tag"
  fi
done