/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file settingsgen.cpp Tool to create computer-readable settings. */

#include "../stdafx.h"
#include "../core/string_consumer.hpp"
#include "../string_func.h"
#include "../strings_type.h"
#include "../misc/getoptdata.h"
#include "../ini_type.h"
#include "../core/mem_func.hpp"
#include "../error_func.h"

#include <filesystem>

#include "../safeguards.h"

/**
 * Report a fatal error.
 * @param s Format string.
 * @note Function does not return.
 */
[[noreturn]] void FatalErrorI(const std::string &msg)
{
	fmt::print(stderr, "settingsgen: FATAL: {}\n", msg);
	exit(1);
}

static const size_t OUTPUT_BLOCK_SIZE = 16000; ///< Block size of the buffer in #OutputBuffer.

/** Output buffer for a block of data. */
class OutputBuffer {
public:
	/** Prepare buffer for use. */
	void Clear()
	{
		this->size = 0;
	}

	/**
	 * Add text to the output buffer.
	 * @param text   Text to store.
	 * @param length Length of the text in bytes.
	 * @return Number of bytes actually stored.
	 */
	size_t Add(std::string_view text)
	{
		size_t store_size = text.copy(this->data + this->size, OUTPUT_BLOCK_SIZE - this->size);
		this->size += store_size;
		return store_size;
	}

	/**
	 * Dump buffer to the output stream.
	 * @param out_fp Stream to write the \a data to.
	 */
	void Write(FILE *out_fp) const
	{
		if (fwrite(this->data, 1, this->size, out_fp) != this->size) {
			FatalError("Cannot write output");
		}
	}

	/**
	 * Does the block have room for more data?
	 * @return \c true if room is available, else \c false.
	 */
	bool HasRoom() const
	{
		return this->size < OUTPUT_BLOCK_SIZE;
	}

	size_t size;                  ///< Number of bytes stored in \a data.
	char data[OUTPUT_BLOCK_SIZE]; ///< Stored data.
};

/** Temporarily store output. */
class OutputStore {
public:
	OutputStore()
	{
		this->Clear();
	}

	/** Clear the temporary storage. */
	void Clear()
	{
		this->output_buffer.clear();
	}

	/**
	 * Add text to the output storage.
	 * @param text Text to store.
	 */
	void Add(std::string_view text)
	{
		if (!text.empty() && this->BufferHasRoom()) {
			size_t stored_size = this->output_buffer[this->output_buffer.size() - 1].Add(text);
			text.remove_prefix(stored_size);
		}
		while (!text.empty()) {
			OutputBuffer &block = this->output_buffer.emplace_back();
			block.Clear(); // Initialize the new block.
			size_t stored_size = block.Add(text);
			text.remove_prefix(stored_size);
		}
	}

	/**
	 * Write all stored output to the output stream.
	 * @param out_fp Stream to write the \a data to.
	 */
	void Write(FILE *out_fp) const
	{
		for (const OutputBuffer &out_data : output_buffer) {
			out_data.Write(out_fp);
		}
	}

private:
	/**
	 * Does the buffer have room without adding a new #OutputBuffer block?
	 * @return \c true if room is available, else \c false.
	 */
	bool BufferHasRoom() const
	{
		size_t num_blocks = this->output_buffer.size();
		return num_blocks > 0 && this->output_buffer[num_blocks - 1].HasRoom();
	}

	typedef std::vector<OutputBuffer> OutputBufferVector; ///< Vector type for output buffers.
	OutputBufferVector output_buffer; ///< Vector of blocks containing the stored output.
};


/** Derived class for loading INI files without going through Fio stuff. */
struct SettingsIniFile : IniLoadFile {
	/**
	 * Construct a new ini loader.
	 * @param list_group_names A list with group names that should be loaded as lists instead of variables. @see IGT_LIST
	 * @param seq_group_names  A list with group names that should be loaded as lists of names. @see IGT_SEQUENCE
	 */
	SettingsIniFile(const IniGroupNameList &list_group_names = {}, const IniGroupNameList &seq_group_names = {}) :
			IniLoadFile(list_group_names, seq_group_names)
	{
	}

	std::optional<FileHandle> OpenFile(std::string_view filename, Subdirectory, size_t *size) override
	{
		/* Open the text file in binary mode to prevent end-of-line translations
		 * done by ftell() and friends, as defined by K&R. */
		auto in = FileHandle::Open(filename, "rb");
		if (!in.has_value()) return in;

		fseek(*in, 0L, SEEK_END);
		*size = ftell(*in);
		fseek(*in, 0L, SEEK_SET); // Seek back to the start of the file.

		return in;
	}

	void ReportFileError(std::string_view pre, std::string_view buffer, std::string_view post) override
	{
		FatalError("{}{}{}", pre, buffer, post);
	}
};

OutputStore _stored_output; ///< Temporary storage of the output, until all processing is done.
OutputStore _post_amble_output; ///< Similar to _stored_output, but for the post amble.

static std::string_view PREAMBLE_GROUP_NAME  = "pre-amble"; ///< Name of the group containing the pre amble.
static std::string_view POSTAMBLE_GROUP_NAME = "post-amble"; ///< Name of the group containing the post amble.
static std::string_view TEMPLATES_GROUP_NAME = "templates"; ///< Name of the group containing the templates.
static std::string_view VALIDATION_GROUP_NAME = "validation"; ///< Name of the group containing the validation statements.
static std::string_view DEFAULTS_GROUP_NAME  = "defaults"; ///< Name of the group containing default values for the template variables.

/**
 * Dump a #IGT_SEQUENCE group into #_stored_output.
 * @param ifile      Loaded INI data.
 * @param group_name Name of the group to copy.
 */
static void DumpGroup(const IniLoadFile &ifile, std::string_view group_name)
{
	const IniGroup *grp = ifile.GetGroup(group_name);
	if (grp != nullptr && grp->type == IGT_SEQUENCE) {
		for (const IniItem &item : grp->items) {
			if (!item.name.empty()) {
				_stored_output.Add(item.name);
				_stored_output.Add("\n");
			}
		}
	}
}

/**
 * Find the value of a template variable.
 * @param name Name of the item to find.
 * @param grp  Group currently being expanded (searched first).
 * @param defaults Fallback group to search, \c nullptr skips the search.
 * @return Text of the item if found, else \c std::nullopt.
 */
static std::optional<std::string_view> FindItemValue(std::string_view name, const IniGroup *grp, const IniGroup *defaults)
{
	const IniItem *item = grp->GetItem(name);
	if (item == nullptr && defaults != nullptr) item = defaults->GetItem(name);
	if (item == nullptr) return std::nullopt;
	return item->value;
}

/**
 * Parse a single entry via a template and output this.
 * @param item The template to use for the output.
 * @param grp Group current being used for template rendering.
 * @param default_grp Default values for items not set in @grp.
 * @param output Output to use for result.
 */
static void DumpLine(const IniItem *item, const IniGroup *grp, const IniGroup *default_grp, OutputStore &output)
{
	/* Prefix with #if/#ifdef/#ifndef */
	static const auto pp_lines = {"if", "ifdef", "ifndef"};
	int count = 0;
	for (const auto &name : pp_lines) {
		auto condition = FindItemValue(name, grp, default_grp);
		if (condition.has_value()) {
			output.Add("#");
			output.Add(name);
			output.Add(" ");
			output.Add(*condition);
			output.Add("\n");
			count++;
		}
	}

	/* Output text of the template, except template variables of the form '$[_a-z0-9]+' which get replaced by their value. */
	static const std::string_view variable_name_characters = "_abcdefghijklmnopqrstuvwxyz0123456789";
	StringConsumer consumer{*item->value};
	while (consumer.AnyBytesLeft()) {
		char c = consumer.ReadChar();
		if (c != '$' || consumer.ReadIf("$")) {
			/* No $ or $$ (literal $). */
			output.Add(std::string_view{&c, 1});
			continue;
		}

		std::string_view variable = consumer.ReadUntilCharNotIn(variable_name_characters);
		if (!variable.empty()) {
			/* Find the text to output. */
			auto valitem = FindItemValue(variable, grp, default_grp);
			if (valitem.has_value()) output.Add(*valitem);
		} else {
			output.Add("$");
		}
	}
	output.Add("\n"); // \n after the expanded template.
	while (count > 0) {
		output.Add("#endif\n");
		count--;
	}
}

/**
 * Output all non-special sections through the template / template variable expansion system.
 * @param ifile Loaded INI data.
 */
static void DumpSections(const IniLoadFile &ifile)
{
	static const auto special_group_names = {PREAMBLE_GROUP_NAME, POSTAMBLE_GROUP_NAME, DEFAULTS_GROUP_NAME, TEMPLATES_GROUP_NAME, VALIDATION_GROUP_NAME};

	const IniGroup *default_grp = ifile.GetGroup(DEFAULTS_GROUP_NAME);
	const IniGroup *templates_grp = ifile.GetGroup(TEMPLATES_GROUP_NAME);
	const IniGroup *validation_grp = ifile.GetGroup(VALIDATION_GROUP_NAME);
	if (templates_grp == nullptr) return;

	/* Output every group, using its name as template name. */
	for (const IniGroup &grp : ifile.groups) {
		/* Exclude special group names. */
		if (std::ranges::find(special_group_names, grp.name) != std::end(special_group_names)) continue;

		const IniItem *template_item = templates_grp->GetItem(grp.name); // Find template value.
		if (template_item == nullptr || !template_item->value.has_value()) {
			FatalError("Cannot find template {}", grp.name);
		}
		DumpLine(template_item, &grp, default_grp, _stored_output);

		if (validation_grp != nullptr) {
			const IniItem *validation_item = validation_grp->GetItem(grp.name); // Find template value.
			if (validation_item != nullptr && validation_item->value.has_value()) {
				DumpLine(validation_item, &grp, default_grp, _post_amble_output);
			}
		}
	}
}

/**
 * Append a file to the output stream.
 * @param fname Filename of file to append.
 * @param out_fp Output stream to write to.
 */
static void AppendFile(std::optional<std::string_view> fname, FILE *out_fp)
{
	if (!fname.has_value()) return;

	auto in_fp = FileHandle::Open(*fname, "r");
	if (!in_fp.has_value()) {
		FatalError("Cannot open file {} for copying", *fname);
	}

	char buffer[4096];
	size_t length;
	do {
		length = fread(buffer, 1, lengthof(buffer), *in_fp);
		if (fwrite(buffer, 1, length, out_fp) != length) {
			FatalError("Cannot copy file");
		}
	} while (length == lengthof(buffer));
}

/**
 * Compare two files for identity.
 * @param n1 First file.
 * @param n2 Second file.
 * @return True if both files are identical.
 */
static bool CompareFiles(std::string_view n1, std::string_view n2)
{
	auto f2 = FileHandle::Open(n2, "rb");
	if (!f2.has_value()) return false;

	auto f1 = FileHandle::Open(n1, "rb");
	if (!f1.has_value()) {
		FatalError("can't open {}", n1);
	}

	size_t l1, l2;
	do {
		char b1[4096];
		char b2[4096];
		l1 = fread(b1, 1, sizeof(b1), *f1);
		l2 = fread(b2, 1, sizeof(b2), *f2);

		if (l1 != l2 || memcmp(b1, b2, l1) != 0) {
			return false;
		}
	} while (l1 != 0);

	return true;
}

/** Options of settingsgen. */
static const OptionData _opts[] = {
	{ .type = ODF_NO_VALUE, .id = 'h', .shortname = 'h', .longname = "--help" },
	{ .type = ODF_NO_VALUE, .id = 'h', .shortname = '?' },
	{ .type = ODF_HAS_VALUE, .id = 'o', .shortname = 'o', .longname = "--output" },
	{ .type = ODF_HAS_VALUE, .id = 'b', .shortname = 'b', .longname = "--before" },
	{ .type = ODF_HAS_VALUE, .id = 'a', .shortname = 'a', .longname = "--after" },
};

/**
 * Process a single INI file.
 * The file should have a [templates] group, where each item is one template.
 * Variables in a template have the form '\$[_a-z0-9]+' (a literal '$' followed
 * by one or more '_', lowercase letters, or lowercase numbers).
 *
 * After loading, the [pre-amble] group is copied verbatim if it exists.
 *
 * For every group with a name that matches a template name the template is written.
 * It starts with a optional \c \#if line if an 'if' item exists in the group. The item
 * value is used as condition. Similarly, \c \#ifdef and \c \#ifndef lines are also written.
 * Below the macro processor directives, the value of the template is written
 * at a line with its variables replaced by item values of the group being written.
 * If the group has no item for the variable, the [defaults] group is tried as fall back.
 * Finally, \c \#endif lines are written to match the macro processor lines.
 *
 * Last but not least, the [post-amble] group is copied verbatim.
 *
 * @param fname  Ini file to process. @return Exit status of the processing.
 */
static void ProcessIniFile(std::string_view fname)
{
	static const IniLoadFile::IniGroupNameList seq_groups = {PREAMBLE_GROUP_NAME, POSTAMBLE_GROUP_NAME};

	SettingsIniFile ini{{}, seq_groups};
	ini.LoadFromDisk(fname, NO_DIRECTORY);

	DumpGroup(ini, PREAMBLE_GROUP_NAME);
	DumpSections(ini);
	DumpGroup(ini, POSTAMBLE_GROUP_NAME);
}

/**
 * And the main program (what else?)
 * @param argc Number of command-line arguments including the program name itself.
 * @param argv Vector of the command-line arguments.
 */
int CDECL main(int argc, char *argv[])
{
	std::optional<std::string_view> output_file;
	std::optional<std::string_view> before_file;
	std::optional<std::string_view> after_file;

	GetOptData mgo(std::span(argv + 1, argc - 1), _opts);
	for (;;) {
		int i = mgo.GetOpt();
		if (i == -1) break;

		switch (i) {
			case 'h':
				fmt::print("settingsgen\n"
						"Usage: settingsgen [options] ini-file...\n"
						"with options:\n"
						"   -h, -?, --help          Print this help message and exit\n"
						"   -b FILE, --before FILE  Copy FILE before all settings\n"
						"   -a FILE, --after FILE   Copy FILE after all settings\n"
						"   -o FILE, --output FILE  Write output to FILE\n");
				return 0;

			case 'o':
				output_file = mgo.opt;
				break;

			case 'a':
				after_file = mgo.opt;
				break;

			case 'b':
				before_file = mgo.opt;
				break;

			case -2:
				fmt::print(stderr, "Invalid arguments\n");
				return 1;
		}
	}

	_stored_output.Clear();
	_post_amble_output.Clear();

	for (auto &argument : mgo.arguments) ProcessIniFile(argument);

	/* Write output. */
	if (!output_file.has_value()) {
		AppendFile(before_file, stdout);
		_stored_output.Write(stdout);
		_post_amble_output.Write(stdout);
		AppendFile(after_file, stdout);
	} else {
		static const std::string_view tmp_output = "tmp2.xxx";

		auto fp = FileHandle::Open(tmp_output, "w");
		if (!fp.has_value()) {
			FatalError("Cannot open file {}", tmp_output);
		}
		AppendFile(before_file, *fp);
		_stored_output.Write(*fp);
		_post_amble_output.Write(*fp);
		AppendFile(after_file, *fp);
		fp.reset();

		std::error_code error_code;
		if (CompareFiles(tmp_output, *output_file)) {
			/* Files are equal. tmp2.xxx is not needed. */
			std::filesystem::remove(tmp_output, error_code);
		} else {
			/* Rename tmp2.xxx to output file. */
			std::filesystem::rename(tmp_output, *output_file, error_code);
			if (error_code) FatalError("rename({}, {}) failed: {}", tmp_output, *output_file, error_code.message());
		}
	}
	return 0;
}

/**
 * Simplified FileHandle::Open which ignores OTTD2FS. Required as settingsgen does not include all of the fileio system.
 * @param filename UTF-8 encoded filename to open.
 * @param mode Mode to open file.
 * @return FileHandle, or std::nullopt on failure.
 */
std::optional<FileHandle> FileHandle::Open(const std::string &filename, std::string_view mode)
{
	auto f = fopen(filename.c_str(), std::string{mode}.c_str());
	if (f == nullptr) return std::nullopt;
	return FileHandle(f);
}
