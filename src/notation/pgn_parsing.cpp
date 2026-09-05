#include "pgn.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <optional>
#include <chrono>
#include <regex>

#include "../game/game.h"
#include "../errors.h"

namespace {

using namespace std::chrono;

GameResult parse_outcome(const std::string& token) {
    if (token == "1-0") { return GameResult::White_by_Unknown; }
    if (token == "0-1") { return GameResult::Black_by_Unknown; }
    if (token == "1/2-1/2") { return GameResult::Draw_by_Unknown; }
    if (token == "*") { return GameResult::Unknown_End; }
    throw PgnError("Result must be \"0-1\", \"1-0\", or \"1/2-1/2\".");
}

int parse_number_string(const std::string& string) {
    if (string.empty()) { throw PgnError("Invalid \"TimeControl\" tag."); }
    if (string.length() > 6) { throw PgnError("Invalid \"TimeControl\" tag."); }
    for (unsigned char c : string) {
        if (!std::isdigit(c)) { throw PgnError("Invalid \"TimeControl\" tag."); }
    }
    return std::stoi(string);
}

std::optional<TimeControl> parse_timecontrol(const std::string& token) {
    if (token == "?" || token == "-") {
        return std::nullopt;
    }

    seconds initial;
    seconds increment;

    std::size_t plus_index = token.find('+');
    if (plus_index == std::string::npos) {
        initial = seconds(parse_number_string(token));
        increment = seconds(0);
    } else {
        initial = seconds(parse_number_string(token.substr(0, plus_index)));
        increment = seconds(parse_number_string(token.substr(plus_index + 1)));
    }

    return TimeControl{duration_cast<milliseconds>(initial), duration_cast<milliseconds>(increment)};
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

    auto at_end = [&] { return pos >= line.size(); };

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
    std::string Event;
    std::string Site;
    std::string White;
    std::string Black;
    GameResult Result;
    std::optional<TimeControl> TimeControl;
    std::optional<Position> FEN;
};

TagPairs parse_tags(const std::vector<std::string>& tags) {
    TagPairs parsed_tags;

    bool found_Event_tag = false;
    bool found_Site_tag = false;
    bool found_White_tag = false;
    bool found_Black_tag = false;
    bool found_Result_tag = false;
    bool found_TimeControl_tag = false;
    bool found_SetUp_tag = false;
    bool found_FEN_tag = false;

    bool expect_FEN_tag = false;

    for (const std::string& line : tags) {
        TagPair tag_value = parse_tag_pair(line);
        const std::string name = tag_value.name;
        const std::string value = tag_value.value;

        if (value.empty()) { throw PgnError("Tag value can't be empty."); }

        if (name == "Event") {
            if (found_Event_tag) {
                throw PgnError("Repeated \"Event\" tag.");
            } else {
                parsed_tags.Event = value;
                found_Event_tag = true;
            }
        } else if (name == "Site") {
            if (found_Site_tag) {
                throw PgnError("Repeated \"Site\" tag.");
            } else {
                parsed_tags.Site = value;
                found_Site_tag = true;
            }
        } else if (name == "White") {
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
        } else if (name == "TimeControl") {
            if (found_TimeControl_tag) {
                throw PgnError("Repeated \"TimeControl\" tag.");
            } else {
                parsed_tags.TimeControl = parse_timecontrol(value);
                found_TimeControl_tag = true;
            }
        } else if (name == "SetUp") {
            if (found_SetUp_tag) {
                throw PgnError("Repeated \"SetUp\" tag.");
            } else {
                if (value == "0") {
                    expect_FEN_tag = false;
                } else if (value ==  "1") {
                    expect_FEN_tag = true;
                } else {
                    throw PgnError("Invalid \"SetUp\" tag value.");
                }
                found_SetUp_tag = true;
            }
        } else if (name == "FEN") {
            if (found_FEN_tag) {
                throw PgnError("Repeated \"FEN\" tag.");
            } else {
                try {
                    parsed_tags.FEN = Position(value); 
                } catch (const FenError& e) {
                    throw PgnError("Invalid fen inside \"FEN\" tag.");
                }
                found_FEN_tag = true;
            }
        }
        // Ignore un-needed PGN tags.
    }

    if (
        !found_Event_tag ||
        !found_Site_tag ||
        !found_White_tag ||
        !found_Black_tag ||
        !found_Result_tag
    ) {
        throw PgnError("Some mandatory tags are not present.");
    }

    if (expect_FEN_tag != found_FEN_tag) {
        throw PgnError("\"SetUp\" tag does not match \"FEN\" tag.");
    }

    return parsed_tags;
}

enum class TokenType { Text, BraceComment };

struct MoveTextToken {
    TokenType type;
    std::string text;
};

std::vector<MoveTextToken> tokenise_movetext(const std::vector<std::string>& movetext) {
    std::vector<MoveTextToken> tokens;

    std::string token;

    int variation_depth = 0;
    bool in_brace_comment = false;

    auto flush_token = [&](TokenType type) {
        if (!token.empty()) {
            tokens.push_back(MoveTextToken{type, std::move(token)});
            token.clear();
        }
    };

    for (const std::string& line : movetext) {

        for (size_t i = 0; i < line.size(); i++) {
            unsigned char c = line[i];

            if (in_brace_comment) {
                if (c == '}') {
                    flush_token(TokenType::BraceComment);
                    in_brace_comment = false;
                } else {
                    token += c;
                }
                continue;
            }

            if (c == '{' && variation_depth == 0) {
                flush_token(TokenType::Text);
                in_brace_comment = true;
                continue;
            }

            if (c == ';' && !in_brace_comment) {
                flush_token(TokenType::Text);
                break; // Ignore remainder of this line.
            }

            if (in_brace_comment) { continue; }

            if (c == '(') {
                variation_depth++;
                flush_token(TokenType::Text);
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
                flush_token(TokenType::Text);
                continue;
            }

            token += c;

            if (
                c == '.' &&
                (i + 1 >= line.size() || line[i + 1] != '.')
            ) {
                flush_token(TokenType::Text);
            }
        }
        
        if (!in_brace_comment) {
            flush_token(TokenType::Text);
        } else {
            token += ' ';
        }
    }

    if (in_brace_comment) { throw PgnError("Unmatched brace comment."); }
    if (variation_depth != 0) { throw PgnError("Unmatched variation."); }

    return tokens;
}

milliseconds parse_hms(const std::string& s) {
    static const std::regex pattern(
        R"(^(\d+):([0-5][0-9]):([0-5][0-9])(?:\.([0-9]{1,3}))?$)"
    );

    std::smatch match;
    if (!std::regex_match(s, match, pattern)) {
        throw PgnError("Malformed clock annotation.");
    }

    int h = 0, m = 0, sec = 0, ms = 0;
    try {
        h = std::stoi(match[1].str());
        m = std::stoi(match[2].str());
        sec = std::stoi(match[3].str());
        if (match[4].matched) {
            std::string frac = match[4].str();
            frac.resize(3, '0'); // pad "5" -> "500", "12" -> "120"
            ms = std::stoi(frac);
        }
    } catch (const std::exception&) {
        // covers overflow from an absurdly long hour string, etc.
        throw PgnError("Malformed clock annotation.");
    }

    return hours(h) + minutes(m) + seconds(sec) + milliseconds(ms);
}

std::optional<milliseconds> parse_clock_comment(const std::string& comment) {
    auto annotation_start = comment.find("[%clk");
    if (annotation_start == std::string::npos) { return std::nullopt; }
    
    auto pos = annotation_start + strlen("[%clk");

    auto at_end = [&] { return pos >= comment.size(); };
    auto character = [&] { return static_cast<unsigned char>(comment[pos]); };

    if (at_end() || !std::isspace(character())) {
        throw PgnError("Malformed clock annotation.");
    }

    std::string clock_string;
    while (character() != ']') {
        if (at_end()) { throw PgnError("Malformed clock annotation."); }
        clock_string += character();
        pos++;
    }

    milliseconds clock_value = parse_hms(trim(clock_string));

    return clock_value;
}

struct MoveTextData {
    std::vector<ParsedPly> plies;
    GameResult outcome;
};

MoveTextData parse_movetext(const std::vector<std::string>& movetext, const Position& starting_pos) {
    MoveTextData parsed_movetext;
    std::vector<MoveTextToken> tokens = tokenise_movetext(movetext);

    if (tokens.empty()) {
        throw PgnError("Movetext section is empty.");
    }

    if (tokens.back().type != TokenType::Text) {
        throw PgnError("Movetext must end with the result.");
    }
    parsed_movetext.outcome = parse_outcome(tokens.back().text);
    tokens.pop_back();

    if (tokens.empty()) {
        return parsed_movetext;
    }

    int expected_move_number = starting_pos.fullmove_number();
    bool expecting_white;

    if (tokens.front().type != TokenType::Text) {
        throw PgnError("Movetext must start with a valid movenumber.");
    }

    if (starting_pos.turn() == Color::White) {
        if (tokens.front().text != std::to_string(expected_move_number) + '.') {
            throw PgnError("Invalid movenumber.");
        }
        expecting_white = true;
    } else {
        if (tokens.front().text != std::to_string(expected_move_number) + "...") {
            throw PgnError("Invalid movenumber.");
        }
        expecting_white = false;
    }
    tokens.erase(tokens.begin());

    bool saw_movenumber_field = true;

    for (const MoveTextToken& token : tokens) {

        if (token.type == TokenType::BraceComment) {
            std::optional<milliseconds> clock_after = parse_clock_comment(token.text);

            if (clock_after) {
                if (parsed_movetext.plies.back().clock_after) {
                    throw PgnError("Clock annotation does not follow a move.");
                }

                parsed_movetext.plies.back().clock_after = clock_after;
            }

            continue;
        }

        if (token.text.starts_with('$')) { continue; }

        if (!saw_movenumber_field) {
            saw_movenumber_field = true;

            if (expecting_white) {
                if (token.text != std::to_string(expected_move_number) + '.') { 
                    throw PgnError("Invalid movenumber.");
                }
                continue;
            } else {
                if (token.text == std::to_string(expected_move_number) + "...") {
                    continue;
                }
            }
        }

        std::string san_move = token.text;

        while (
            !san_move.empty() &&
            (san_move.back() == '!' ||
            san_move.back() == '?')
        ) {
            san_move.pop_back();
        }

        std::replace(san_move.begin(), san_move.end(), '0', 'O'); // normalize castling

        parsed_movetext.plies.push_back(ParsedPly{san_move, std::nullopt});

        saw_movenumber_field = false;
        if (expecting_white) {
            expecting_white = false;
        } else {
            expecting_white = true;
            expected_move_number++;
        }
    }

    if (saw_movenumber_field) {
        throw PgnError("Movenumber doesn't follow a move.");
    }

    return parsed_movetext;
}

};

ParsedPGN parse_pgn_document(const std::vector<std::string>& lines) {
    ParsedPGN parsed;
    PgnSections sections = separate_sections(lines);

    TagPairs tag_pairs = parse_tags(sections.tags);
    parsed.event = tag_pairs.Event;
    parsed.site = tag_pairs.Site;
    parsed.white_name = tag_pairs.White;
    parsed.black_name = tag_pairs.Black;
    parsed.result = tag_pairs.Result;
    parsed.time_control = tag_pairs.TimeControl;
    parsed.starting_position = tag_pairs.FEN;

    MoveTextData movetext = parse_movetext(
        sections.movetext,
        parsed.starting_position ? *parsed.starting_position : Position()
    );
    parsed.plies = std::move(movetext.plies);
    if (parsed.result != movetext.outcome) {
        throw PgnError("Tag and Movetext results disagree.");
    }

    return parsed;
}
