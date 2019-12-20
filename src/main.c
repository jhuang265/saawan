#include <stdio.h>
#include <string.h>
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "view.h"
#include "libpdf/hpdf.h"
#include "layout.h"
#include "pdf.h"
#include "draw.h"
#include "util.h"
#include "fonts.h"
#include "error.h"
#include "templates.h"

#ifdef LIBXML_TREE_ENABLED

// XML String Constants
#define XML_FONT		 			(const xmlChar*) "Font"
#define XML_FONT_REGULAR			(const xmlChar*) "regular"
#define XML_FONT_BOLD				(const xmlChar*) "bold"
#define XML_FONT_ITALIC				(const xmlChar*) "italic"
#define XML_FONT_BOLDITALIC			(const xmlChar*) "bolditalic"
#define XML_NAME					(const xmlChar*) "name"
#define XML_PAGE		 			(const xmlChar*) "Page"
#define XML_TEMPLATE	 			(const xmlChar*) "Template"

// Defining from pdf.h
HPDF_Doc pdf;

typedef struct xml_doc_node {
	xmlDoc* elem;
	struct xml_doc_node* next;
} xml_doc_node;

xml_doc_node* xml_docs = NULL;

void usage() {
	printf("Usage: saawan [file]\n");
	printf("	file: a valid Saawan XML Layout File\n");
}

void error_handler (HPDF_STATUS   error_no,
	HPDF_STATUS   detail_no,
	void         *user_data)
{
	printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
		(HPDF_UINT)detail_no);
	exit(1);
}

void process_file(char* filename) {
	printf("Processing file: %s...\n", filename);
	xmlDoc* doc = NULL;
	xmlNode* root_element = NULL;

	doc = xmlReadFile(filename, "UTF-8", 0);

	if (doc == NULL) {
		fprintf(stderr, "Could not parse file!\n");
		exit(1);
	}

	// Add to LL
	xml_doc_node* doc_node = malloc(sizeof(xml_doc_node));
	doc_node->elem = doc;
	doc_node->next = xml_docs;
	xml_docs = doc_node;

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);
	if (xmlStrcmp(root_element->name, (const xmlChar *) "Document")) {
		fprintf(stderr, "Document of the wrong type, your root node must be a 'Document' object!\n");
		exit(1);
	}

	if (!pdf) {
		fprintf(stderr, "Cannot create pdf object.\n");
		exit(1);
	}

	for (xmlNode* child = root_element->children; child; child = child->next) {
		if (xmlIsBlankNode(child)) continue;
		if (child->type == XML_ELEMENT_NODE) {
			if (xmlStrEqual(child->name, XML_FONT)) {
				xmlChar* name = xmlGetProp(child, XML_NAME);
				if (!name) {
					error(child->line, FONT_REQUIRES_NAME);
				}
				printf("Loading Font: %s...\n", name);
				xmlChar* regular_path = xmlGetProp(child, XML_FONT_REGULAR);
				xmlChar* bold_path = xmlGetProp(child, XML_FONT_BOLD);
				xmlChar* italic_path = xmlGetProp(child, XML_FONT_ITALIC);
				xmlChar* bolditalic_path = xmlGetProp(child, XML_FONT_BOLDITALIC);

				if (regular_path) {
					put_font(name, STYLE_NONE, regular_path, child->line);
					free(regular_path);
				}
				if (bold_path) {
					put_font(name, STYLE_BOLD, bold_path, child->line);
					free(bold_path);
				}
				if (italic_path) {
					put_font(name, STYLE_ITALIC, italic_path, child->line);
					free(italic_path);
				}
				if (bolditalic_path) {
					put_font(name, STYLE_BOLDITALIC, bolditalic_path, child->line);
					free(bolditalic_path);
				}
				free(name);
			}
			else if (xmlStrEqual(child->name, XML_PAGE)) {
				printf("Building Page...\n");
				HPDF_Page page = HPDF_AddPage(pdf);
				HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
				// root_element is DOCUMENT, first child is a page, loop through here
				view* root = build_page(child);
				measure_and_layout(root);
				// print_view(root);

				draw(root, page);
				free_view(root);
			}
			else if (xmlStrEqual(child->name, XML_TEMPLATE)) {
				add_template(child, child->line);
			}
		}
	}
}

int main(int argc, char **argv) {
	if (argc == 1) {
		usage();
		return 1;
	}
	LIBXML_TEST_VERSION
	pdf = HPDF_New(error_handler, NULL);
	HPDF_UseUTFEncodings(pdf);
	HPDF_SetCurrentEncoder(pdf, "UTF-8");
	char* write_to_name = NULL;
	fonts_init();
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp("-b", argv[i]) == 0 ||
				strcmp("--show-bounding-box", argv[i]) == 0) {
				set_settings_flag(SETTINGS_SHOW_BOUNDING_BOX);
			}
			else if (strcmp("-o", argv[i]) == 0 && i < argc - 1) {
				write_to_name = argv[i + 1];
				i++;
			}
			else {
				usage();
				exit(1);
			}
		}
		else {
			process_file(argv[i]);
		}
    }

	free_templates_ll();

	while (xml_docs) {
		xmlFreeDoc(xml_docs->elem);
		xml_doc_node* next = xml_docs->next;
		free(xml_docs);
		xml_docs = next;
	}

	xmlCleanupParser();
	if (!write_to_name) {
		write_to_name = "out.pdf";
	}
	printf("Writing to file: %s...\n", write_to_name);
	HPDF_SaveToFile(pdf, write_to_name);
	HPDF_Free(pdf);
	free_fonts_ll();
	printf("Done!\n");
	return 0;
}
#else
int main(void) {
	fprintf(stderr, "Tree support not compiled in\n");
	exit(1);
}
#endif