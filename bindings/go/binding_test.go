package tree_sitter_kak_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_kak "github.com/saifulapm/tree-sitter-kak/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_kak.Language())
	if language == nil {
		t.Errorf("Error loading Kak grammar")
	}
}
