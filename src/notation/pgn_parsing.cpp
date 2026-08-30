#include "pgn.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

#include "../game/game.h"
#include "../errors.h"

namespace {

GameResult parse_outcome(std::string_view token) {
    if (token == "1-0") { return GameResult::White_by_Unknown; }
    if (token == "0-1") { return GameResult::Black_by_Unknown; }
    if (token == "1/2-1/2") { return GameResult::Draw_by_Unknown; }
    throw PgnError("Result must be \"0-1\", \"1-0\", or \"1/2-1/2\".");
}

enum class PgnSection { TagsSection, MoveTextSection };

struct PgnSections {
    std::vector<std::string> tags;
    std::vector<std::string> movetext;
};

std::string trim(const std::string& str) {
    const auto start = str.find_first_not_of(" \t\n\r");
    const auto end = str.find_last_not_of(" \t\n\r");
    
    if (start == std::string::npos) { return ""; }
    
    return str.substr(start, end - start + 1);
}

PgnSections separate_sections(const std::vector<std::string>& lines) {
    PgnSection current_section = PgnSection::TagsSection;
    PgnSections sections;

    for (const std::string& line : lines) {
        const std::string trimmed = trim(line);
        if (trimmed.empty()) { continue; }

        if (current_section == PgnSection::TagsSection) {
            if (trimmed.starts_with('[')) {
                if (!trimmed.ends_with(']')) {
                    throw PgnError("Tag pair must end with ']'.");
                }
                sections.tags.push_back(trimmed);
            } else {
                current_section = PgnSection::MoveTextSection;
                sections.movetext.push_back(trimmed);
            }
        } else {
            sections.movetext.push_back(trimmed);
        }
    }

    return sections;
}

struct TagPair {
    std::string name;
    std::string value;
};

TagPair parse_tag_pair(std::string_view line) {
    if (line.empty()) { throw PgnError("Invalid tag pair."); }
    if (line[0] != '[') { throw PgnError("Invalid tag pair."); }

    std::size_t pos = 1;

    auto at_end = [&] {
        return pos >= line.size();
    };

    auto is_supported_char = [](char c) {
        return (
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_'
        );
    };

    while (!at_end() && !std::isspace(line[pos])) {
        if (!is_supported_char(line[pos])) { throw PgnError("Invalid tag pair."); }
        pos++;
    }

    std::string name{line.substr(1, pos - 1)};

    // At least one whitespace character.
    if (at_end() || !std::isspace(line[pos])) { throw PgnError("Invalid tag pair."); } 
    while (!at_end() && std::isspace(line[pos])) { pos++; }

    // Opening quote must appear immediately after the whitespace.
    if (at_end() || line[pos] != '"') { throw PgnError("Invalid tag pair."); }
    pos++;

    std::string value;
    bool found_closing_quote = false;

    while (!at_end()) {
        char c = line[pos++];

        if (c == '\\') {
            if (at_end()) { throw PgnError("Incomplete escape sequence."); }

            char escaped = line[pos++];
            if (escaped != '\\' && escaped != '"') {
                throw PgnError("Unsupported escape sequence.");
            }

            value += escaped;
        } else if (c == '"') {
            found_closing_quote = true;
            break;
        } else {
            value += c;
        }
    }

    if (!found_closing_quote) { throw PgnError("Invalid tag pair."); }

    while (!at_end() && std::isspace(line[pos])) { pos++; }

    if (at_end() || line[pos] != ']') { throw PgnError("Invalid tag pair."); }
    pos++;
    if (!at_end()) { throw PgnError("Invalid tag pair."); }

    return {std::move(name), std::move(value)};
}

struct TagPairs {
    std::string White;
    std::string Black;
    GameResult Result;
};

TagPairs parse_tags(const std::vector<std::string>& tags) {
    TagPairs parsed_tags;

    bool found_White_tag = false;
    bool found_Black_tag = false;
    bool found_Result_tag = false;

    for (const std::string& line : tags) {
        TagPair tag_value = parse_tag_pair(line);
        const std::string name = tag_value.name;
        const std::string value = tag_value.value;

        if (value.empty()) { throw PgnError("Tag value can't be empty."); }

        if (name == "White") {
            if (found_White_tag) {
                throw PgnError("Repeated \"White\" tag.");
            } else {
                parsed_tags.White = value;
                found_White_tag = true;
            }
        } else if (name == "Black") {
            if (found_Black_tag) {
                throw PgnError("Repeated \"Black\" tag.");
            } else {
                parsed_tags.Black = value;
                found_Black_tag = true;
            }
        } else if (name == "Result") {
            if (found_Result_tag) {
                throw PgnError("Repeated \"Result\" tag.");
            } else {
                parsed_tags.Result = parse_outcome(value);
                found_Result_tag = true;
            }
        }
        // Ignore un-needed PGN tags.
    }

    if (!found_White_tag || !found_Black_tag || !found_Result_tag) {
        throw PgnError("Some mandatory tags are not present.");
    }

    return parsed_tags;
}

std::vector<std::string> tokenise_movetext(const std::vector<std::string>& movetext) {
    std::vector<std::string> tokens;

    int variation_depth = 0;
    bool in_brace_comment = false;
    for (const std::string& line : movetext) {
        std::string token;
        auto flush_token = [&] {
            if (!token.empty()) {
                tokens.push_back(std::move(token));
                token.clear();
            }
        };

        for (size_t i = 0; i < line.size(); i++) {
            unsigned char c = line[i];

            if (c == ';' && !in_brace_comment) {
                flush_token();
                break; // Ignore remainder of this line.
            }

            if (c == '{') {
                if (in_brace_comment) {
                    throw PgnError("Unmatched brace comment.");
                }
                in_brace_comment = true;
                flush_token();
                continue;
            } else if (c == '}') {
                if (!in_brace_comment) {
                    throw PgnError("Unmatched brace comment.");
                }
                in_brace_comment = false;
                continue;
            }

            if (in_brace_comment) { continue; }

            if (c == '(') {
                variation_depth++;
                flush_token();
                continue;
            } else if (c == ')') {
                if (variation_depth == 0) {
                    throw PgnError("Unmatched variation.");
                }
                variation_depth--;
                continue;
            }

            if (variation_depth > 0) { continue; }

            if (std::isspace(c)) {
                flush_token();
                continue;
            }

            token += c;

            if (
                c == '.' &&
                (i + 1 >= line.size() || line[i + 1] != '.')
            ) {
                flush_token();
            }
        }
        
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    if (in_brace_comment) { throw PgnError("Unmatched brace comment."); }
    if (variation_depth != 0) { throw PgnError("Unmatched variation."); }

    return tokens;
}

struct MoveTextData {
    std::vector<std::string> san_moves;
    GameResult outcome;
};

MoveTextData parse_movetext(const std::vector<std::string>& movetext) {
    MoveTextData parsed_movetext;
    std::vector<std::string> tokens = tokenise_movetext(movetext);

    if (tokens.empty()) {
        throw PgnError("Movetext section is empty.");
    }

    parsed_movetext.outcome = parse_outcome(tokens.back());
    tokens.pop_back();

    int expected_move_number = 1;
    bool expecting_white = true;
    bool saw_movenumber_field = false;
    for (const std::string& token : tokens) {
        if (token.starts_with('$')) { continue; }

        if (!saw_movenumber_field) {
            saw_movenumber_field = true;

            if (expecting_white) {
                if (token != std::to_string(expected_move_number) + '.') { 
                    throw PgnError("Invalid movenumber.");
                }
                continue;
            } else {
                if (token == std::to_string(expected_move_number) + "...") {
                    continue;
                }
            }
        }

        std::string san_move = token;

        while (
            !san_move.empty() &&
            (san_move.back() == '!' ||
            san_move.back() == '?')
        ) {
            san_move.pop_back();
        }

        std::replace(san_move.begin(), san_move.end(), '0', 'O'); // normalize castling

        parsed_movetext.san_moves.push_back(san_move);

        saw_movenumber_field = false;
        if (expecting_white) {
            expecting_white = false;
        } else {
            expecting_white = true;
            expected_move_number++;
        }
    }

    return parsed_movetext;
}

};

ParsedPGN parse_pgn_document(const std::vector<std::string>& lines) {
    ParsedPGN parsed;
    PgnSections sections = separate_sections(lines);

    TagPairs tag_pairs = parse_tags(sections.tags);
    parsed.white_name = tag_pairs.White;
    parsed.black_name = tag_pairs.Black;
    parsed.result = tag_pairs.Result;

    MoveTextData movetext = parse_movetext(sections.movetext);
    parsed.san_moves = std::move(movetext.san_moves);
    if (parsed.result != movetext.outcome) {
        throw PgnError("Tag and Movetext results disagree.");
    }

    return parsed;
}
