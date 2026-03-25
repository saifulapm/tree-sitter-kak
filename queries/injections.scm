; Inject bash into %sh{...} expansion content
((expansion
  type: (expansion_type) @_type
  content: (string_content) @injection.content)
 (#eq? @_type "sh")
 (#set! injection.language "bash"))
