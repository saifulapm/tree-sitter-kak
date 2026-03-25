import XCTest
import SwiftTreeSitter
import TreeSitterKakscript

final class TreeSitterKakscriptTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_kakscript())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Kakscript grammar")
    }
}
