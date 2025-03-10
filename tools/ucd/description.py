
import sys

class description (object):
    def __init__(self):
        # UnicodeData.txt
        self.bidi_class = "ON"
        self.canonical_combining_class = 0
        self.decomposition_type = None
        self.decomposition_mapping = []
        self.general_category = "Cn"
        self.line_break = "XX"
        self.bidi_mirrored = False
        self.upper_cast_mapping = []
        self.lower_cast_mapping = []
        self.title_cast_mapping = []

        # BidiBrackets.txt
        self.bidi_paired_bracket_type = "n"
        self.bidi_paired_bracket = None

        # BidiMirroring.txt
        self.bidi_mirroring_glyph = None

        # CompositionExclusions.txt
        self.composition_exclusion = False

        # EastAsianWidth.txt
        self.east_asian_width = "N"

        # GraphemeBreakProperty.txt
        self.grapheme_cluster_break = "Other"

        # SentenceBreakProperty.txt
        self.sentence_break = "Other"

        # WordBreakProperty.txt
        self.word_break = "Other"

        # LineBreak.txt
        self.line_break = "XX"

        # Scripts.txt
        self.script = "Zzzz"

        # emoji-data.txt
        self.emoji = False
        self.emoji_presentation = False
        self.emoji_modifier = False
        self.emoji_modifier_base = False
        self.emoji_component = False
        self.extended_pictographic = False

        # PropList.txt
        self.white_space = False
        self.bidi_control = False
        self.join_control = False
        self.dash = False
        self.hyphen = False
        self.quotation_mark = False
        self.terminal_punctuation = False
        self.other_math = False
        self.hex_digit = False
        self.ascii_hex_digit = False
        self.other_alphabetic = False
        self.ideographic = False
        self.diacritic = False
        self.extender = False
        self.other_lowercase = False
        self.other_uppercase = False
        self.non_character = False
        self.other_grapheme_extend = False
        self.ids_binary_operator = False
        self.ids_trinary_operator = False
        self.radical = False
        self.unified_ideograph = False
        self.other_default_ignorable = False
        self.deprecated = False
        self.soft_dotted = False
        self.logical_order_exception = False
        self.other_id_start = False
        self.other_id_continue = False
        self.sentence_terminal = False
        self.variation_selector = False
        self.pattern_white_space = False
        self.pattern_syntax = False
        self.prepended_concatenation_mark = False
        self.regional_indicator = False

def initialize_descriptions():
    """Initialize a table with description for all 0x110000 code-points.
    """
    print("Initialize descriptions for 0x110000 code-points.", file=sys.stderr, flush=True)
    return [description() for i in range(0x110000)]

