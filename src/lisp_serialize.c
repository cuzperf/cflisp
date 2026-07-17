#include "lisp_internal.h"

#define IMAGE_MAGIC     ((uint32_t)0x4D444C46U)  /* "FLDM" little-endian */
#define IMAGE_VERSION   ((uint32_t)1U)

/* ================================================================
 *  Simple Map: void* key -> int32_t value
 * ================================================================ */
typedef struct {
    void*   key;
    int32_t val;
} Entry;

typedef struct {
    Entry*  entries;
    int     count;
    int     capacity;
} Map;

static void map_init(Map* m, int cap)
{
    m->entries  = (Entry*)malloc((size_t)cap * sizeof(Entry));
    m->count    = 0;
    m->capacity = cap;
}

static void map_free(Map* m)
{
    free(m->entries);
    m->entries  = NULL;
    m->count    = 0;
    m->capacity = 0;
}

static int32_t map_get(const Map* m, void* key)
{
    for (int i = 0; i < m->count; ++i) {
        if (m->entries[i].key == key)
            return m->entries[i].val;
    }
    return -1;
}

static void map_put(Map* m, void* key, int32_t val)
{
    if (m->count >= m->capacity) {
        m->capacity *= 2;
        m->entries = (Entry*)realloc(m->entries, (size_t)m->capacity * sizeof(Entry));
    }
    m->entries[m->count].key = key;
    m->entries[m->count].val = val;
    ++m->count;
}

/* ================================================================
 *  Symbol collection (in-order BST traversal)
 * ================================================================ */
static void collect_symbols(Symbol* s, Map* sym_map)
{
    if (!s) return;
    collect_symbols(s->left, sym_map);
    map_put(sym_map, s, (int32_t)sym_map->count);
    collect_symbols(s->right, sym_map);
}

/* ================================================================
 *  Cell collection from symbol table bindings + transitive closure
 * ================================================================ */
static void add_cell_if_list(value_t v, Map* cell_map)
{
    if (v == UNBOUND) return;
    if (type_of(v) != TYPE_LIST || v == EMPTY_LIST) return;
    void* p = ptr(v);
    if (map_get(cell_map, p) < 0)
        map_put(cell_map, p, (int32_t)cell_map->count);
}

static void collect_root_cells(Symbol* s, Map* cell_map)
{
    if (!s) return;
    add_cell_if_list(s->binding, cell_map);
    collect_root_cells(s->left, cell_map);
    collect_root_cells(s->right, cell_map);
}

static void collect_all_cells(Map* cell_map)
{
    int i = 0;
    while (i < cell_map->count) {
        List*   cell = (List*)cell_map->entries[i].key;
        value_t h    = cell->head;
        value_t t    = cell->tail;
        add_cell_if_list(h, cell_map);
        add_cell_if_list(t, cell_map);
        ++i;
    }
}

/* ================================================================
 *  Serialized value I/O
 * ================================================================ */
static void write_value(FILE* f, value_t v, const Map* sym_map, const Map* cell_map)
{
    uint32_t tag;
    int32_t  data;

    if (v == UNBOUND) {
        tag  = (uint32_t)TYPE_SYM;
        data = -1;
    } else {
        tag = (uint32_t)type_of(v);
        switch (tag) {
        case TYPE_NUM:
            data = cf_num_val(v);
            break;
        case TYPE_LIST:
            data = (v == EMPTY_LIST) ? -1 : map_get(cell_map, ptr(v));
            break;
        case TYPE_SYM:
            data = map_get(sym_map, ptr(v));
            break;
        case TYPE_BUILTIN:
            data = (int32_t)builtin_val(v)->code;
            break;
        default:
            tag  = (uint32_t)TYPE_SYM;
            data = -1;
            break;
        }
    }

    fwrite(&tag,  sizeof(uint32_t), 1, f);
    fwrite(&data, sizeof(int32_t),  1, f);
}

static int read_pending(FILE* f, uint32_t* tag, int32_t* data)
{
    if (fread(tag, sizeof(uint32_t), 1, f) != 1) return 0;
    if (fread(data, sizeof(int32_t), 1, f) != 1) return 0;
    return 1;
}

static value_t resolve_value(uint32_t tag, int32_t data, Symbol** syms, int cell_ss)
{
    switch (tag) {
    case TYPE_NUM:     return number(data);
    case TYPE_LIST:    return (data == -1) ? EMPTY_LIST : g_stack[cell_ss + data];
    case TYPE_SYM:     return (data == -1) ? UNBOUND : tagptr(syms[data], TAG_SYM);
    case TYPE_BUILTIN: return tagptr(&g_builtins[data], TAG_OTHER);
    default:           return UNBOUND;
    }
}

/* ================================================================
 *  Public API
 * ================================================================ */
CF_API void cf_lisp_serialize(const char* filename)
{
    /* --- collect symbols --- */
    Map sym_map;
    map_init(&sym_map, 64);
    collect_symbols(symtab, &sym_map);

    /* --- collect all reachable cells --- */
    Map cell_map;
    map_init(&cell_map, 256);
    collect_root_cells(symtab, &cell_map);
    collect_all_cells(&cell_map);

    /* --- write image --- */
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "--- failed to open output image file: %s ---\n", filename);
        map_free(&cell_map);
        map_free(&sym_map);
        return;
    }

    uint32_t magic      = IMAGE_MAGIC;
    uint32_t version    = IMAGE_VERSION;
    uint32_t num_syms   = (uint32_t)sym_map.count;
    uint32_t num_cells  = (uint32_t)cell_map.count;

    fwrite(&magic,      sizeof(uint32_t), 1, f);
    fwrite(&version,    sizeof(uint32_t), 1, f);
    fwrite(&num_syms,   sizeof(uint32_t), 1, f);
    fwrite(&num_cells,  sizeof(uint32_t), 1, f);

    /* symbol table */
    for (int i = 0; i < sym_map.count; ++i) {
        Symbol*  s        = (Symbol*)sym_map.entries[i].key;
        uint16_t name_len = (uint16_t)strlen(s->name);
        fwrite(&name_len, sizeof(uint16_t), 1, f);
        fwrite(s->name, 1, name_len, f);
        write_value(f, s->binding, &sym_map, &cell_map);
    }

    /* cell table */
    for (int i = 0; i < cell_map.count; ++i) {
        List* cell = (List*)cell_map.entries[i].key;
        write_value(f, cell->head, &sym_map, &cell_map);
        write_value(f, cell->tail, &sym_map, &cell_map);
    }

    fclose(f);
    map_free(&cell_map);
    map_free(&sym_map);

    printf("--- serialized %u symbols and %u cells to %s ---\n",
           num_syms, num_cells, filename);
}

CF_API bool cf_lisp_deserialize_image(const char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (!f) return false;

    /* --- read header --- */
    uint32_t magic, version, num_syms, num_cells;
    if (fread(&magic,   sizeof(uint32_t), 1, f) != 1) goto fail;
    if (fread(&version, sizeof(uint32_t), 1, f) != 1) goto fail;
    if (magic != IMAGE_MAGIC || version != IMAGE_VERSION) goto fail;
    if (fread(&num_syms,  sizeof(uint32_t), 1, f) != 1) goto fail;
    if (fread(&num_cells, sizeof(uint32_t), 1, f) != 1) goto fail;

    /* --- allocate pending arrays --- */
    Symbol**  syms              = (Symbol**) calloc(num_syms,  sizeof(Symbol*));
    uint32_t* sym_binding_tags  = (uint32_t*)malloc(num_syms  * sizeof(uint32_t));
    int32_t*  sym_binding_data  = (int32_t*) malloc(num_syms  * sizeof(int32_t));
    uint32_t* cell_head_tags    = (uint32_t*)malloc(num_cells * sizeof(uint32_t));
    int32_t*  cell_head_data    = (int32_t*) malloc(num_cells * sizeof(int32_t));
    uint32_t* cell_tail_tags    = (uint32_t*)malloc(num_cells * sizeof(uint32_t));
    int32_t*  cell_tail_data    = (int32_t*) malloc(num_cells * sizeof(int32_t));

    /* --- read symbol entries --- */
    for (uint32_t i = 0; i < num_syms; ++i) {
        uint16_t name_len;
        if (fread(&name_len, sizeof(uint16_t), 1, f) != 1) goto fail_free;
        char* name = (char*)malloc((size_t)name_len + 1);
        if (fread(name, 1, name_len, f) != name_len) { free(name); goto fail_free; }
        name[name_len] = '\0';

        value_t sv = symbol(name, &symtab);
        syms[i] = sym_val(sv);
        free(name);

        if (!read_pending(f, &sym_binding_tags[i], &sym_binding_data[i]))
            goto fail_free;
    }

    /* --- read cell entries (pending) --- */
    for (uint32_t i = 0; i < num_cells; ++i) {
        if (!read_pending(f, &cell_head_tags[i], &cell_head_data[i])) goto fail_free;
        if (!read_pending(f, &cell_tail_tags[i], &cell_tail_data[i])) goto fail_free;
    }

    fclose(f);
    f = NULL;

    /* --- allocate cells on GC heap, protect on value stack --- */
    int cell_ss = g_sp;
    for (uint32_t i = 0; i < num_cells; ++i) {
        value_t cell = make_cell(UNBOUND);
        push(cell);
    }

    /* --- resolve symbol bindings --- */
    for (uint32_t i = 0; i < num_syms; ++i) {
        syms[i]->binding = resolve_value(sym_binding_tags[i], sym_binding_data[i],
                                         syms, cell_ss);
    }

    /* --- resolve cell contents --- */
    for (uint32_t i = 0; i < num_cells; ++i) {
        List* cell   = list_val(g_stack[cell_ss + i]);
        cell->head = resolve_value(cell_head_tags[i], cell_head_data[i], syms, cell_ss);
        cell->tail = resolve_value(cell_tail_tags[i], cell_tail_data[i], syms, cell_ss);
    }

    /* --- cleanup --- */
    restore_stack(cell_ss);

    free(syms);
    free(sym_binding_tags);
    free(sym_binding_data);
    free(cell_head_tags);
    free(cell_head_data);
    free(cell_tail_tags);
    free(cell_tail_data);

    printf("--- deserialized image %s (%u symbols, %u cells) ---\n",
           filename, num_syms, num_cells);
    return true;

fail_free:
    free(syms);
    free(sym_binding_tags);
    free(sym_binding_data);
    free(cell_head_tags);
    free(cell_head_data);
    free(cell_tail_tags);
    free(cell_tail_data);
fail:
    if (f) fclose(f);
    return false;
}
