curl -X 'GET' 'http://localhost:5000/mecs/1/types' -H 'accept: application/json' > type.json 2>/dev/null
diff <(jq --sort-keys . type.json) <(jq --sort-keys . references/type.json) >/dev/null
echo $?
