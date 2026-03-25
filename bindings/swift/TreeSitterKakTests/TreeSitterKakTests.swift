import XCTest
import SwiftTreeSitter
import TreeSitterKak

final class TreeSitterKakTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_kak())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Kak grammar")
    }
}
