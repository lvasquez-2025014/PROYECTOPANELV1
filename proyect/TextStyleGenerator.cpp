#include "TextStyleGenerator.hpp"
#include <cctype>
#include <algorithm>

namespace TextStyle {

    // ===== MAPEAMENTOS DE CARACTERES =====

    // Bold (𝐀𝐁𝐂𝐃𝐄𝐅𝐆𝐇𝐈𝐉𝐊𝐋𝐌𝐍𝐎𝐏𝐐𝐑𝐒𝐓𝐔𝐕𝐖𝐗𝐘𝐙𝐚𝐛𝐜𝐝𝐞𝐟𝐠𝐡𝐢𝐣𝐤𝐥𝐦𝐧𝐨𝐩𝐪𝐫𝐬𝐭𝐮𝐯𝐰𝐱𝐲𝐳𝟎𝟏𝟐𝟑𝟒𝟓𝟔𝟕𝟖𝟗)
    const std::unordered_map<char, std::string> StyleConverter::boldMap = {
        {'A', "𝐀"}, {'B', "𝐁"}, {'C', "𝐂"}, {'D', "𝐃"}, {'E', "𝐄"}, {'F', "𝐅"},
        {'G', "𝐆"}, {'H', "𝐇"}, {'I', "𝐈"}, {'J', "𝐉"}, {'K', "𝐊"}, {'L', "𝐋"},
        {'M', "𝐌"}, {'N', "𝐍"}, {'O', "𝐎"}, {'P', "𝐏"}, {'Q', "𝐐"}, {'R', "𝐑"},
        {'S', "𝐒"}, {'T', "𝐓"}, {'U', "𝐔"}, {'V', "𝐕"}, {'W', "𝐖"}, {'X', "𝐗"},
        {'Y', "𝐘"}, {'Z', "𝐙"},
        {'a', "𝐚"}, {'b', "𝐛"}, {'c', "𝐜"}, {'d', "𝐝"}, {'e', "𝐞"}, {'f', "𝐟"},
        {'g', "𝐠"}, {'h', "𝐡"}, {'i', "𝐢"}, {'j', "𝐣"}, {'k', "𝐤"}, {'l', "𝐥"},
        {'m', "𝐦"}, {'n', "𝐧"}, {'o', "𝐨"}, {'p', "𝐩"}, {'q', "𝐪"}, {'r', "𝐫"},
        {'s', "𝐬"}, {'t', "𝐭"}, {'u', "𝐮"}, {'v', "𝐯"}, {'w', "𝐰"}, {'x', "𝐱"},
        {'y', "𝐲"}, {'z', "𝐳"},
        {'0', "𝟎"}, {'1', "𝟏"}, {'2', "𝟐"}, {'3', "𝟑"}, {'4', "𝟒"}, {'5', "𝟓"},
        {'6', "𝟔"}, {'7', "𝟕"}, {'8', "𝟖"}, {'9', "𝟗"}
    };

    // Italic (𝘈𝘉𝘊𝘋𝘌𝘍𝘎𝘏𝘐𝘑𝘒𝘓𝘔𝘕𝘖𝘗𝘘𝘙𝘚𝘛𝘜𝘝𝘞𝘟𝘠𝘡𝘢𝘣𝘤𝘥𝘦𝘧𝘨𝘩𝘪𝘫𝘬𝘭𝘮𝘯𝘰𝘱𝘲𝘳𝘴𝘵𝘶𝘷𝘸𝘹𝘺𝘻)
    const std::unordered_map<char, std::string> StyleConverter::italicMap = {
        {'A', "𝘈"}, {'B', "𝘉"}, {'C', "𝘊"}, {'D', "𝘋"}, {'E', "𝘌"}, {'F', "𝘍"},
        {'G', "𝘎"}, {'H', "𝘏"}, {'I', "𝘐"}, {'J', "𝘑"}, {'K', "𝘒"}, {'L', "𝘓"},
        {'M', "𝘔"}, {'N', "𝘕"}, {'O', "𝘖"}, {'P', "𝘗"}, {'Q', "𝘘"}, {'R', "𝘙"},
        {'S', "𝘚"}, {'T', "𝘛"}, {'U', "𝘜"}, {'V', "𝘝"}, {'W', "𝘞"}, {'X', "𝘟"},
        {'Y', "𝘠"}, {'Z', "𝘡"},
        {'a', "𝘢"}, {'b', "𝘣"}, {'c', "𝘤"}, {'d', "𝘥"}, {'e', "𝘦"}, {'f', "𝘧"},
        {'g', "𝘨"}, {'h', "𝘩"}, {'i', "𝘪"}, {'j', "𝘫"}, {'k', "𝘬"}, {'l', "𝘭"},
        {'m', "𝘮"}, {'n', "𝘯"}, {'o', "𝘰"}, {'p', "𝘱"}, {'q', "𝘲"}, {'r', "𝘳"},
        {'s', "𝘴"}, {'t', "𝘵"}, {'u', "𝘶"}, {'v', "𝘷"}, {'w', "𝘸"}, {'x', "𝘹"},
        {'y', "𝘺"}, {'z', "𝘻"}
    };

    // Bold Italic (𝙖𝙗𝙘𝙙𝙚𝙛𝙜𝙝𝙞𝙟𝙠𝙡𝙢𝙣𝙤𝙥𝙦𝙧𝙨𝙩𝙪𝙫𝙬𝙭𝙮𝙯)
    const std::unordered_map<char, std::string> StyleConverter::boldItalicMap = {
        {'A', "𝙖"}, {'B', "𝙗"}, {'C', "𝙘"}, {'D', "𝙙"}, {'E', "𝙚"}, {'F', "𝙛"},
        {'G', "𝙜"}, {'H', "𝙝"}, {'I', "𝙞"}, {'J', "𝙟"}, {'K', "𝙠"}, {'L', "𝙡"},
        {'M', "𝙢"}, {'N', "𝙣"}, {'O', "𝙤"}, {'P', "𝙥"}, {'Q', "𝙦"}, {'R', "𝙧"},
        {'S', "𝙨"}, {'T', "𝙩"}, {'U', "𝙪"}, {'V', "𝙫"}, {'W', "𝙬"}, {'X', "𝙭"},
        {'Y', "𝙮"}, {'Z', "𝙯"},
        {'a', "𝙖"}, {'b', "𝙗"}, {'c', "𝙘"}, {'d', "𝙙"}, {'e', "𝙚"}, {'f', "𝙛"},
        {'g', "𝙜"}, {'h', "𝙝"}, {'i', "𝙞"}, {'j', "𝙟"}, {'k', "𝙠"}, {'l', "𝙡"},
        {'m', "𝙢"}, {'n', "𝙣"}, {'o', "𝙤"}, {'p', "𝙥"}, {'q', "𝙦"}, {'r', "𝙧"},
        {'s', "𝙨"}, {'t', "𝙩"}, {'u', "𝙪"}, {'v', "𝙫"}, {'w', "𝙬"}, {'x', "𝙭"},
        {'y', "𝙮"}, {'z', "𝙯"}
    };

    // Fraktur (𝔞𝔟𝔠𝔡𝔢𝔣𝔤𝔥𝔦𝔧𝔨𝔩𝔪𝔫𝔬𝔭𝔮𝔯𝔰𝔱𝔲𝔳𝔴𝔵𝔶𝔷)
    const std::unordered_map<char, std::string> StyleConverter::frakturMap = {
        {'A', "𝔄"}, {'B', "𝔅"}, {'C', "ℭ"}, {'D', "𝔇"}, {'E', "𝔈"}, {'F', "𝔉"},
        {'G', "𝔊"}, {'H', "ℌ"}, {'I', "ℑ"}, {'J', "𝔍"}, {'K', "𝔎"}, {'L', "𝔏"},
        {'M', "𝔐"}, {'N', "𝔑"}, {'O', "𝔒"}, {'P', "𝔓"}, {'Q', "𝔔"}, {'R', "ℜ"},
        {'S', "𝔖"}, {'T', "𝔗"}, {'U', "𝔘"}, {'V', "𝔙"}, {'W', "𝔚"}, {'X', "𝔛"},
        {'Y', "𝔜"}, {'Z', "ℨ"},
        {'a', "𝔞"}, {'b', "𝔟"}, {'c', "𝔠"}, {'d', "𝔡"}, {'e', "𝔢"}, {'f', "𝔣"},
        {'g', "𝔤"}, {'h', "𝔥"}, {'i', "𝔦"}, {'j', "𝔧"}, {'k', "𝔨"}, {'l', "𝔩"},
        {'m', "𝔪"}, {'n', "𝔫"}, {'o', "𝔬"}, {'p', "𝔭"}, {'q', "𝔮"}, {'r', "𝔯"},
        {'s', "𝔰"}, {'t', "𝔱"}, {'u', "𝔲"}, {'v', "𝔳"}, {'w', "𝔴"}, {'x', "𝔵"},
        {'y', "𝔶"}, {'z', "𝔷"}
    };

    // Double Struck (𝕒𝕓𝕔𝕕𝕖𝕗𝕘𝕙𝕚𝕛𝕜𝕝𝕞𝕟𝕠𝕡𝕢𝕣𝕤𝕥𝕦𝕧𝕨𝕩𝕪𝕫)
    const std::unordered_map<char, std::string> StyleConverter::doubleStruckMap = {
        {'A', "𝔸"}, {'B', "𝔹"}, {'C', "ℂ"}, {'D', "𝔻"}, {'E', "𝔼"}, {'F', "𝔽"},
        {'G', "𝔾"}, {'H', "ℍ"}, {'I', "𝕀"}, {'J', "𝕁"}, {'K', "𝕂"}, {'L', "𝕃"},
        {'M', "𝕄"}, {'N', "ℕ"}, {'O', "𝕆"}, {'P', "ℙ"}, {'Q', "ℚ"}, {'R', "ℝ"},
        {'S', "𝕊"}, {'T', "𝕋"}, {'U', "𝕌"}, {'V', "𝕍"}, {'W', "𝕎"}, {'X', "𝕏"},
        {'Y', "𝕐"}, {'Z', "ℤ"},
        {'a', "𝕒"}, {'b', "𝕓"}, {'c', "𝕔"}, {'d', "𝕕"}, {'e', "𝕖"}, {'f', "𝕗"},
        {'g', "𝕘"}, {'h', "𝕙"}, {'i', "𝕚"}, {'j', "𝕛"}, {'k', "𝕜"}, {'l', "𝕝"},
        {'m', "𝕞"}, {'n', "𝕟"}, {'o', "𝕠"}, {'p', "𝕡"}, {'q', "𝕢"}, {'r', "𝕣"},
        {'s', "𝕤"}, {'t', "𝕥"}, {'u', "𝕦"}, {'v', "𝕧"}, {'w', "𝕨"}, {'x', "𝕩"},
        {'y', "𝕪"}, {'z', "𝕫"}
    };

    // Bubble (ⓐⓑⓒⓓⓔⓕⓖⓗⓘⓙⓚⓛⓜⓝⓞⓟⓠⓡⓢⓣⓤⓥⓦⓧⓨⓩ)
    const std::unordered_map<char, std::string> StyleConverter::bubbleMap = {
        {'A', "Ⓐ"}, {'B', "Ⓑ"}, {'C', "Ⓒ"}, {'D', "Ⓓ"}, {'E', "Ⓔ"}, {'F', "Ⓕ"},
        {'G', "Ⓖ"}, {'H', "Ⓗ"}, {'I', "Ⓘ"}, {'J', "Ⓙ"}, {'K', "Ⓚ"}, {'L', "Ⓛ"},
        {'M', "Ⓜ"}, {'N', "Ⓝ"}, {'O', "Ⓞ"}, {'P', "Ⓟ"}, {'Q', "Ⓠ"}, {'R', "Ⓡ"},
        {'S', "Ⓢ"}, {'T', "Ⓣ"}, {'U', "Ⓤ"}, {'V', "Ⓥ"}, {'W', "Ⓦ"}, {'X', "Ⓧ"},
        {'Y', "Ⓨ"}, {'Z', "Ⓩ"},
        {'a', "ⓐ"}, {'b', "ⓑ"}, {'c', "ⓒ"}, {'d', "ⓓ"}, {'e', "ⓔ"}, {'f', "ⓕ"},
        {'g', "ⓖ"}, {'h', "ⓗ"}, {'i', "ⓘ"}, {'j', "ⓙ"}, {'k', "ⓚ"}, {'l', "ⓛ"},
        {'m', "ⓜ"}, {'n', "ⓝ"}, {'o', "ⓞ"}, {'p', "ⓟ"}, {'q', "ⓠ"}, {'r', "ⓡ"},
        {'s', "ⓢ"}, {'t', "ⓣ"}, {'u', "ⓤ"}, {'v', "ⓥ"}, {'w', "ⓦ"}, {'x', "ⓧ"},
        {'y', "ⓨ"}, {'z', "ⓩ"},
        {'0', "⓪"}, {'1', "①"}, {'2', "②"}, {'3', "③"}, {'4', "④"}, {'5', "⑤"},
        {'6', "⑥"}, {'7', "⑦"}, {'8', "⑧"}, {'9', "⑨"}
    };

    // Black Bubble (🅐🅑🅒🅓🅔🅕🅖🅗🅘🅙🅚🅛🅜🅝🅞🅟🅠🅡🅢🅣🅤🅥🅦🅧🅨🅩)
    const std::unordered_map<char, std::string> StyleConverter::blackBubbleMap = {
        {'A', "🅐"}, {'B', "🅑"}, {'C', "🅒"}, {'D', "🅓"}, {'E', "🅔"}, {'F', "🅕"},
        {'G', "🅖"}, {'H', "🅗"}, {'I', "🅘"}, {'J', "🅙"}, {'K', "🅚"}, {'L', "🅛"},
        {'M', "🅜"}, {'N', "🅝"}, {'O', "🅞"}, {'P', "🅟"}, {'Q', "🅠"}, {'R', "🅡"},
        {'S', "🅢"}, {'T', "🅣"}, {'U', "🅤"}, {'V', "🅥"}, {'W', "🅦"}, {'X', "🅧"},
        {'Y', "🅨"}, {'Z', "🅩"}
    };

    // Monospace (𝚊𝚋𝚌𝚍𝚎𝚏𝚐𝚑𝚒𝚓𝚔𝚕𝚖𝚗𝚘𝚙𝚚𝚛𝚜𝚝𝚞𝚟𝚠𝚡𝚢𝚣)
    const std::unordered_map<char, std::string> StyleConverter::monospaceMap = {
        {'A', "𝙰"}, {'B', "𝙱"}, {'C', "𝙲"}, {'D', "𝙳"}, {'E', "𝙴"}, {'F', "𝙵"},
        {'G', "𝙶"}, {'H', "𝙷"}, {'I', "𝙸"}, {'J', "𝙹"}, {'K', "𝙺"}, {'L', "𝙻"},
        {'M', "𝙼"}, {'N', "𝙽"}, {'O', "𝙾"}, {'P', "𝙿"}, {'Q', "𝚀"}, {'R', "𝚁"},
        {'S', "𝚂"}, {'T', "𝚃"}, {'U', "𝚄"}, {'V', "𝚅"}, {'W', "𝚆"}, {'X', "𝚇"},
        {'Y', "𝚈"}, {'Z', "𝚉"},
        {'a', "𝚊"}, {'b', "𝚋"}, {'c', "𝚌"}, {'d', "𝚍"}, {'e', "𝚎"}, {'f', "𝚏"},
        {'g', "𝚐"}, {'h', "𝚑"}, {'i', "𝚒"}, {'j', "𝚓"}, {'k', "𝚔"}, {'l', "𝚕"},
        {'m', "𝚖"}, {'n', "𝚗"}, {'o', "𝚘"}, {'p', "𝚙"}, {'q', "𝚚"}, {'r', "𝚛"},
        {'s', "𝚜"}, {'t', "𝚝"}, {'u', "𝚞"}, {'v', "𝚟"}, {'w', "𝚠"}, {'x', "𝚡"},
        {'y', "𝚢"}, {'z', "𝚣"},
        {'0', "𝟶"}, {'1', "𝟷"}, {'2', "𝟸"}, {'3', "𝟹"}, {'4', "𝟺"}, {'5', "𝟻"},
        {'6', "𝟼"}, {'7', "𝟽"}, {'8', "𝟾"}, {'9', "𝟿"}
    };

    // Small Caps (ᴀʙᴄᴅᴇꜰɢʜɪᴊᴋʟᴍɴᴏᴘǫʀsᴛᴜᴠᴡxʏᴢ)
    const std::unordered_map<char, std::string> StyleConverter::smallCapsMap = {
        {'A', "ᴀ"}, {'B', "ʙ"}, {'C', "ᴄ"}, {'D', "ᴅ"}, {'E', "ᴇ"}, {'F', "ꜰ"},
        {'G', "ɢ"}, {'H', "ʜ"}, {'I', "ɪ"}, {'J', "ᴊ"}, {'K', "ᴋ"}, {'L', "ʟ"},
        {'M', "ᴍ"}, {'N', "ɴ"}, {'O', "ᴏ"}, {'P', "ᴘ"}, {'Q', "ǫ"}, {'R', "ʀ"},
        {'S', "s"}, {'T', "ᴛ"}, {'U', "ᴜ"}, {'V', "ᴠ"}, {'W', "ᴡ"}, {'X', "x"},
        {'Y', "ʏ"}, {'Z', "ᴢ"},
        {'a', "ᴀ"}, {'b', "ʙ"}, {'c', "ᴄ"}, {'d', "ᴅ"}, {'e', "ᴇ"}, {'f', "ꜰ"},
        {'g', "ɢ"}, {'h', "ʜ"}, {'i', "ɪ"}, {'j', "ᴊ"}, {'k', "ᴋ"}, {'l', "ʟ"},
        {'m', "ᴍ"}, {'n', "ɴ"}, {'o', "ᴏ"}, {'p', "ᴘ"}, {'q', "ǫ"}, {'r', "ʀ"},
        {'s', "s"}, {'t', "ᴛ"}, {'u', "ᴜ"}, {'v', "ᴠ"}, {'w', "ᴡ"}, {'x', "x"},
        {'y', "ʏ"}, {'z', "ᴢ"}
    };

    // Tiny (ᴀʙᴄᴅᴇꜰɢʜɪᴊᴋʟᴍɴᴏᴘǫʀsᴛᴜᴠᴡxʏᴢ) - Similar to small caps
    const std::unordered_map<char, std::string> StyleConverter::tinyMap = smallCapsMap;

    // Upside Down (ɐqɔpǝɟƃɥᴉɾʞlɯuodbɹsʇnʌʍxʎz)
    const std::unordered_map<char, std::string> StyleConverter::upsideDownMap = {
        {'A', "∀"}, {'B', "q"}, {'C', "Ɔ"}, {'D', "p"}, {'E', "Ǝ"}, {'F', "Ⅎ"},
        {'G', "⅁"}, {'H', "H"}, {'I', "I"}, {'J', "ſ"}, {'K', "⋊"}, {'L', "˥"},
        {'M', "W"}, {'N', "N"}, {'O', "O"}, {'P', "Ԁ"}, {'Q', "Ὸ"}, {'R', "ᴚ"},
        {'S', "S"}, {'T', "⊥"}, {'U', "∩"}, {'V', "Λ"}, {'W', "M"}, {'X', "X"},
        {'Y', "⅄"}, {'Z', "Z"},
        {'a', "ɐ"}, {'b', "q"}, {'c', "ɔ"}, {'d', "p"}, {'e', "ǝ"}, {'f', "ɟ"},
        {'g', "ƃ"}, {'h', "ɥ"}, {'i', "ᴉ"}, {'j', "ɾ"}, {'k', "ʞ"}, {'l', "l"},
        {'m', "ɯ"}, {'n', "u"}, {'o', "o"}, {'p', "d"}, {'q', "b"}, {'r', "ɹ"},
        {'s', "s"}, {'t', "ʇ"}, {'u', "n"}, {'v', "ʌ"}, {'w', "ʍ"}, {'x', "x"},
        {'y', "ʎ"}, {'z', "z"},
        {'0', "0"}, {'1', "Ɩ"}, {'2', "ᄅ"}, {'3', "Ɛ"}, {'4', "ㄣ"}, {'5', "ϛ"},
        {'6', "9"}, {'7', "ㄥ"}, {'8', "8"}, {'9', "6"}
    };

    // Fancy Style 1 (яεsυℓтα∂σ)
    const std::unordered_map<char, std::string> StyleConverter::fancyStyle1Map = {
        {'A', "α"}, {'B', "в"}, {'C', "ς"}, {'D', "∂"}, {'E', "ε"}, {'F', "ƒ"},
        {'G', "ƃ"}, {'H', "н"}, {'I', "ι"}, {'J', "ј"}, {'K', "κ"}, {'L', "ℓ"},
        {'M', "м"}, {'N', "η"}, {'O', "σ"}, {'P', "ρ"}, {'Q', "q"}, {'R', "я"},
        {'S', "s"}, {'T', "τ"}, {'U', "υ"}, {'V', "ν"}, {'W', "ω"}, {'X', "χ"},
        {'Y', "ψ"}, {'Z', "ζ"},
        {'a', "α"}, {'b', "в"}, {'c', "ς"}, {'d', "∂"}, {'e', "ε"}, {'f', "ƒ"},
        {'g', "ƃ"}, {'h', "н"}, {'i', "ι"}, {'j', "ј"}, {'k', "κ"}, {'l', "ℓ"},
        {'m', "м"}, {'n', "η"}, {'o', "σ"}, {'p', "ρ"}, {'q', "q"}, {'r', "я"},
        {'s', "s"}, {'t', "τ"}, {'u', "υ"}, {'v', "ν"}, {'w', "ω"}, {'x', "χ"},
        {'y', "ψ"}, {'z', "ζ"}
    };

    // Fancy Style 2 (尺乇丂ㄩㄥㄒ卂ᗪㄖ)
    const std::unordered_map<char, std::string> StyleConverter::fancyStyle2Map = {
        {'A', "卂"}, {'B', "乃"}, {'C', "匚"}, {'D', "ᗪ"}, {'E', "乇"}, {'F', "千"},
        {'G', "ᘜ"}, {'H', "卄"}, {'I', "丨"}, {'J', "ﾌ"}, {'K', "Ҝ"}, {'L', "ㄥ"},
        {'M', "爪"}, {'N', "几"}, {'O', "ㄖ"}, {'P', "卩"}, {'Q', "Ɋ"}, {'R', "尺"},
        {'S', "丂"}, {'T', "ㄒ"}, {'U', "ㄩ"}, {'V', "ᐯ"}, {'W', "山"}, {'X', "乂"},
        {'Y', "ㄚ"}, {'Z', "乙"},
        {'a', "卂"}, {'b', "乃"}, {'c', "匚"}, {'d', "ᗪ"}, {'e', "乇"}, {'f', "千"},
        {'g', "ᘜ"}, {'h', "卄"}, {'i', "丨"}, {'j', "ﾌ"}, {'k', "Ҝ"}, {'l', "ㄥ"},
        {'m', "爪"}, {'n', "几"}, {'o', "ㄖ"}, {'p', "卩"}, {'q', "Ɋ"}, {'r', "尺"},
        {'s', "丂"}, {'t', "ㄒ"}, {'u', "ㄩ"}, {'v', "ᐯ"}, {'w', "山"}, {'x', "乂"},
        {'y', "ㄚ"}, {'z', "乙"}
    };

    // Magic (ꮢꭼsuꮮꮖꭺꭰꮎ)
    const std::unordered_map<char, std::string> StyleConverter::magicMap = {
        {'A', "ꭺ"}, {'B', "ꭱ"}, {'C', "ꭲ"}, {'D', "ꭰ"}, {'E', "ꭼ"}, {'F', "ꭿ"},
        {'G', "ꮁ"}, {'H', "ꮋ"}, {'I', "ꮎ"}, {'J', "ꮏ"}, {'K', "ꮗ"}, {'L', "ꮮ"},
        {'M', "ꮇ"}, {'N', "ꮑ"}, {'O', "ꮎ"}, {'P', "ꮲ"}, {'Q', "ꮕ"}, {'R', "ꮢ"},
        {'S', "ꮥ"}, {'T', "ꮦ"}, {'U', "ꮼ"}, {'V', "ꮙ"}, {'W', "ꮃ"}, {'X', "ꮪ"},
        {'Y', "ꮰ"}, {'Z', "ꮖ"},
        {'a', "ꭺ"}, {'b', "ꭱ"}, {'c', "ꭲ"}, {'d', "ꭰ"}, {'e', "ꭼ"}, {'f', "ꭿ"},
        {'g', "ꮁ"}, {'h', "ꮋ"}, {'i', "ꮎ"}, {'j', "ꮏ"}, {'k', "ꮗ"}, {'l', "ꮮ"},
        {'m', "ꮇ"}, {'n', "ꮑ"}, {'o', "ꮎ"}, {'p', "ꮲ"}, {'q', "ꮕ"}, {'r', "ꮢ"},
        {'s', "ꮥ"}, {'t', "ꮦ"}, {'u', "ꮼ"}, {'v', "ꮙ"}, {'w', "ꮃ"}, {'x', "ꮪ"},
        {'y', "ꮰ"}, {'z', "ꮖ"}
    };

    // Full Width (ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ)
    const std::unordered_map<char, std::string> StyleConverter::fullWidthMap = {
        {'A', "Ａ"}, {'B', "Ｂ"}, {'C', "Ｃ"}, {'D', "Ｄ"}, {'E', "Ｅ"}, {'F', "Ｆ"},
        {'G', "Ｇ"}, {'H', "Ｈ"}, {'I', "Ｉ"}, {'J', "Ｊ"}, {'K', "Ｋ"}, {'L', "Ｌ"},
        {'M', "Ｍ"}, {'N', "Ｎ"}, {'O', "Ｏ"}, {'P', "Ｐ"}, {'Q', "Ｑ"}, {'R', "Ｒ"},
        {'S', "Ｓ"}, {'T', "Ｔ"}, {'U', "Ｕ"}, {'V', "Ｖ"}, {'W', "Ｗ"}, {'X', "Ｘ"},
        {'Y', "Ｙ"}, {'Z', "Ｚ"},
        {'a', "ａ"}, {'b', "ｂ"}, {'c', "ｃ"}, {'d', "ｄ"}, {'e', "ｅ"}, {'f', "ｆ"},
        {'g', "ｇ"}, {'h', "ｈ"}, {'i', "ｉ"}, {'j', "ｊ"}, {'k', "ｋ"}, {'l', "ｌ"},
        {'m', "ｍ"}, {'n', "ｎ"}, {'o', "ｏ"}, {'p', "ｐ"}, {'q', "ｑ"}, {'r', "ｒ"},
        {'s', "ｓ"}, {'t', "ｔ"}, {'u', "ｕ"}, {'v', "ｖ"}, {'w', "ｗ"}, {'x', "ｘ"},
        {'y', "ｙ"}, {'z', "ｚ"},
        {'0', "０"}, {'1', "１"}, {'2', "２"}, {'3', "３"}, {'4', "４"}, {'5', "５"},
        {'6', "６"}, {'7', "７"}, {'8', "８"}, {'9', "９"},
        {' ', "　"}
    };

    // Square (🅁🄴🅂🅄🄻🅃🄰🄳🄾)
    const std::unordered_map<char, std::string> StyleConverter::squareMap = {
        {'A', "🄰"}, {'B', "🄱"}, {'C', "🄲"}, {'D', "🄳"}, {'E', "🄴"}, {'F', "🄵"},
        {'G', "🄶"}, {'H', "🄷"}, {'I', "🄸"}, {'J', "🄹"}, {'K', "🄺"}, {'L', "🄻"},
        {'M', "🄼"}, {'N', "🄽"}, {'O', "🄾"}, {'P', "🄿"}, {'Q', "🅀"}, {'R', "🅁"},
        {'S', "🅂"}, {'T', "🅃"}, {'U', "🅄"}, {'V', "🅅"}, {'W', "🅆"}, {'X', "🅇"},
        {'Y', "🅈"}, {'Z', "🅉"}
    };

    // Strikethrough (a̶b̶c̶...)
    const std::unordered_map<char, std::string> StyleConverter::strikethroughMap = {
        {'A', "A̶"}, {'B', "B̶"}, {'C', "C̶"}, {'D', "D̶"}, {'E', "E̶"}, {'F', "F̶"},
        {'G', "G̶"}, {'H', "H̶"}, {'I', "I̶"}, {'J', "J̶"}, {'K', "K̶"}, {'L', "L̶"},
        {'M', "M̶"}, {'N', "N̶"}, {'O', "O̶"}, {'P', "P̶"}, {'Q', "Q̶"}, {'R', "R̶"},
        {'S', "S̶"}, {'T', "T̶"}, {'U', "U̶"}, {'V', "V̶"}, {'W', "W̶"}, {'X', "X̶"},
        {'Y', "Y̶"}, {'Z', "Z̶"},
        {'a', "a̶"}, {'b', "b̶"}, {'c', "c̶"}, {'d', "d̶"}, {'e', "e̶"}, {'f', "f̶"},
        {'g', "g̶"}, {'h', "h̶"}, {'i', "i̶"}, {'j', "j̶"}, {'k', "k̶"}, {'l', "l̶"},
        {'m', "m̶"}, {'n', "n̶"}, {'o', "o̶"}, {'p', "p̶"}, {'q', "q̶"}, {'r', "r̶"},
        {'s', "s̶"}, {'t', "t̶"}, {'u', "u̶"}, {'v', "v̶"}, {'w', "w̶"}, {'x', "x̶"},
        {'y', "y̶"}, {'z', "z̶"}
    };

    // Underline (a̲b̲c̲...)
    const std::unordered_map<char, std::string> StyleConverter::underlineMap = {
        {'A', "A̲"}, {'B', "B̲"}, {'C', "C̲"}, {'D', "D̲"}, {'E', "E̲"}, {'F', "F̲"},
        {'G', "G̲"}, {'H', "H̲"}, {'I', "I̲"}, {'J', "J̲"}, {'K', "K̲"}, {'L', "L̲"},
        {'M', "M̲"}, {'N', "N̲"}, {'O', "O̲"}, {'P', "P̲"}, {'Q', "Q̲"}, {'R', "R̲"},
        {'S', "S̲"}, {'T', "T̲"}, {'U', "U̲"}, {'V', "V̲"}, {'W', "W̲"}, {'X', "X̲"},
        {'Y', "Y̲"}, {'Z', "Z̲"},
        {'a', "a̲"}, {'b', "b̲"}, {'c', "c̲"}, {'d', "d̲"}, {'e', "e̲"}, {'f', "f̲"},
        {'g', "g̲"}, {'h', "h̲"}, {'i', "i̲"}, {'j', "j̲"}, {'k', "k̲"}, {'l', "l̲"},
        {'m', "m̲"}, {'n', "n̲"}, {'o', "o̲"}, {'p', "p̲"}, {'q', "q̲"}, {'r', "r̲"},
        {'s', "s̲"}, {'t', "t̲"}, {'u', "u̲"}, {'v', "v̲"}, {'w', "w̲"}, {'x', "x̲"},
        {'y', "y̲"}, {'z', "z̲"}
    };

    // ===== IMPLEMENTAÇO DAS FUNÇÕES =====

    std::string StyleConverter::convertChar(char c, Style style) {
        if (c == ' ') return " ";
        
        switch (style) {
            case Style::BOLD:
                return boldMap.count(c) ? boldMap.at(c) : std::string(1, c);
            case Style::ITALIC:
                return italicMap.count(c) ? italicMap.at(c) : std::string(1, c);
            case Style::BOLD_ITALIC:
                return boldItalicMap.count(c) ? boldItalicMap.at(c) : std::string(1, c);
            case Style::FRAKTUR:
                return frakturMap.count(c) ? frakturMap.at(c) : std::string(1, c);
            case Style::BOLD_FRAKTUR:
                return frakturMap.count(c) ? frakturMap.at(c) : std::string(1, c);
            case Style::DOUBLE_STRUCK:
                return doubleStruckMap.count(c) ? doubleStruckMap.at(c) : std::string(1, c);
            case Style::BUBBLE:
                return bubbleMap.count(c) ? bubbleMap.at(c) : std::string(1, c);
            case Style::BLACK_BUBBLE:
                return blackBubbleMap.count(c) ? blackBubbleMap.at(c) : std::string(1, c);
            case Style::MONOSPACE:
                return monospaceMap.count(c) ? monospaceMap.at(c) : std::string(1, c);
            case Style::SMALL_CAPS:
                return smallCapsMap.count(c) ? smallCapsMap.at(c) : std::string(1, c);
            case Style::TINY:
                return tinyMap.count(c) ? tinyMap.at(c) : std::string(1, c);
            case Style::UPSIDE_DOWN:
                return upsideDownMap.count(c) ? upsideDownMap.at(c) : std::string(1, c);
            case Style::FANCY_STYLE_1:
                return fancyStyle1Map.count(c) ? fancyStyle1Map.at(c) : std::string(1, c);
            case Style::FANCY_STYLE_2:
                return fancyStyle2Map.count(c) ? fancyStyle2Map.at(c) : std::string(1, c);
            case Style::MAGIC:
                return magicMap.count(c) ? magicMap.at(c) : std::string(1, c);
            case Style::FULL_WIDTH:
                return fullWidthMap.count(c) ? fullWidthMap.at(c) : std::string(1, c);
            case Style::SQUARE:
                return squareMap.count(c) ? squareMap.at(c) : std::string(1, c);
            case Style::STRIKETHROUGH:
                return strikethroughMap.count(c) ? strikethroughMap.at(c) : std::string(1, c);
            case Style::UNDERLINE:
                return underlineMap.count(c) ? underlineMap.at(c) : std::string(1, c);
            default:
                return std::string(1, c);
        }
    }

    std::string StyleConverter::convert(const std::string& text, Style style) {
        std::string result;
        for (char c : text) {
            result += convertChar(c, style);
        }
        return result;
    }

    std::vector<std::string> StyleConverter::convertMultiple(const std::string& text, 
                                                             const std::vector<Style>& styles) {
        std::vector<std::string> results;
        for (const auto& style : styles) {
            results.push_back(convert(text, style));
        }
        return results;
    }

    std::string StyleConverter::getStyleName(Style style) {
        switch (style) {
            case Style::BOLD: return "Bold";
            case Style::ITALIC: return "Italic";
            case Style::BOLD_ITALIC: return "Bold Italic";
            case Style::FRAKTUR: return "Fraktur";
            case Style::BOLD_FRAKTUR: return "Bold Fraktur";
            case Style::DOUBLE_STRUCK: return "Double Struck";
            case Style::BUBBLE: return "Bubble";
            case Style::BLACK_BUBBLE: return "Black Bubble";
            case Style::MONOSPACE: return "Monospace";
            case Style::SMALL_CAPS: return "Small Caps";
            case Style::TINY: return "Tiny";
            case Style::UPSIDE_DOWN: return "Upside Down";
            case Style::FANCY_STYLE_1: return "Fancy Style 1";
            case Style::FANCY_STYLE_2: return "Fancy Style 2";
            case Style::MAGIC: return "Magic";
            case Style::FULL_WIDTH: return "Full Width";
            case Style::SQUARE: return "Square";
            case Style::STRIKETHROUGH: return "Strikethrough";
            case Style::UNDERLINE: return "Underline";
            default: return "Normal";
        }
    }

    std::vector<StyleConverter::Style> StyleConverter::getAllStyles() {
        return {
            Style::BOLD,
            Style::ITALIC,
            Style::BOLD_ITALIC,
            Style::FRAKTUR,
            Style::BOLD_FRAKTUR,
            Style::DOUBLE_STRUCK,
            Style::BUBBLE,
            Style::BLACK_BUBBLE,
            Style::MONOSPACE,
            Style::SMALL_CAPS,
            Style::TINY,
            Style::UPSIDE_DOWN,
            Style::FANCY_STYLE_1,
            Style::FANCY_STYLE_2,
            Style::MAGIC,
            Style::FULL_WIDTH,
            Style::SQUARE,
            Style::STRIKETHROUGH,
            Style::UNDERLINE
        };
    }

    // ===== DIACRÍTICOS =====

    std::string DiacriticsApplier::getDiacriticChar(DiacriticType type) {
        switch (type) {
            case DiacriticType::BRIDGE_ABOVE: return "͆͆";
            case DiacriticType::ASTERISK_BELOW: return "͙͙";
            case DiacriticType::PLUS_SIGN_BELOW: return "̟̟";
            case DiacriticType::X_ABOVE_BELOW: return "͓͓̽̽";
            case DiacriticType::BRIDGE_BELOW: return "̺̺";
            case DiacriticType::UPWARD_ARROW_BELOW: return "͎͎";
            case DiacriticType::STRIKETHROUGH: return "̶̶";
            case DiacriticType::SLASH: return "̷̷";
            case DiacriticType::DOUBLE_UNDERLINE: return "̳̳";
            case DiacriticType::LOVE_HEARTS: return "♥♥";
            case DiacriticType::INVISIBLE_INK: return "҉҉";
            default: return "";
        }
    }

    std::string DiacriticsApplier::applyDiacritic(const std::string& text, DiacriticType type) {
        std::string diacritic = getDiacriticChar(type);
        std::string result;
        for (char c : text) {
            result += c;
            result += diacritic;
        }
        return result;
    }

    // ===== GERADORES ESPECIAIS =====

    std::string SpecialGenerators::addInvisibleSpaces(const std::string& text, int spacing) {
        std::string result;
        for (size_t i = 0; i < text.length(); ++i) {
            result += text[i];
            if (i < text.length() - 1) {
                for (int j = 0; j < spacing; ++j) {
                    result += "ㅤ";
                }
            }
        }
        return result;
    }

    std::string SpecialGenerators::addDecorativeSymbols(const std::string& text, 
                                                       const std::string& symbol) {
        return symbol + " " + text + " " + symbol;
    }

    std::string SpecialGenerators::addRandomEmojis(const std::string& text) {
        static const std::vector<std::string> emojis = {
            "✿", "♬︎", "☆", "亗", "〆", "☃︎", "⚡", "❤", "💔", "💍"
        };
        std::string result;
        for (size_t i = 0; i < text.length(); ++i) {
            result += text[i];
            if (i < text.length() - 1) {
                result += emojis[i % emojis.size()];
            }
        }
        return result;
    }

    std::string SpecialGenerators::addFFColor(const std::string& text, const std::string& colorCode) {
        return "[" + colorCode + "]" + text;
    }

    std::string SpecialGenerators::combineStyles(const std::string& text, 
                                                 const std::vector<StyleConverter::Style>& styles) {
        std::string result = text;
        for (const auto& style : styles) {
            result = StyleConverter::convert(result, style);
        }
        return result;
    }

    // ===== UTILITÁRIOS =====

    bool Utils::isUpperCase(char c) {
        return c >= 'A' && c <= 'Z';
    }

    bool Utils::isLowerCase(char c) {
        return c >= 'a' && c <= 'z';
    }

    bool Utils::isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    bool Utils::isLetter(char c) {
        return isUpperCase(c) || isLowerCase(c);
    }

    char Utils::toUpper(char c) {
        return isLowerCase(c) ? c - 32 : c;
    }

    char Utils::toLower(char c) {
        return isUpperCase(c) ? c + 32 : c;
    }

    std::string Utils::trimSpaces(const std::string& text) {
        size_t start = text.find_first_not_of(" \t\n\r");
        size_t end = text.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        return text.substr(start, end - start + 1);
    }

    bool Utils::isValidFFLength(const std::string& text) {
        return text.length() <= 50;
    }

    int Utils::getEffectiveLength(const std::string& text) {
        // Contar apenas caracteres visíveis (não diacríticos)
        int count = 0;
        for (size_t i = 0; i < text.length(); ++i) {
            unsigned char c = text[i];
            // Pular bytes de continuação UTF-8
            if ((c & 0xC0) != 0x80) {
                count++;
            }
        }
        return count;
    }

} // namespace TextStyle
