#include <string.h>
#include <ndctl/libndctl.h>
#include <ndctl/libdaxctl.h>

#define BUS_MAX 2
#define REGION_MAX 2
#define NAMESPACES_MAX 2
#define SETS_MAX REGION_MAX
#define DIMM_MAX 2
#define DEVDAX_MAX 2

enum namespace_type {
	PFN,
	DEVDAX,
	BLOCK,
	BTT
};

struct daxctl_dev {
	char name[PATH_MAX];
	struct daxctl_dev *next;
};

struct daxctl_region {
	struct daxctl_dev devdax[DEVDAX_MAX];
};

struct ndctl_pfn {
	char name[PATH_MAX];
};

struct ndctl_btt {
	char name[PATH_MAX];
};

struct ndctl_dax {
	struct daxctl_region region;
};

struct ndctl_namespace {
	char name[PATH_MAX];
	struct ndctl_namespace *next;
	enum namespace_type type;
	union {
		struct ndctl_pfn pfn;
		struct ndctl_btt btt;
		struct ndctl_dax dax;
	};
};

struct ndctl_dimm {
	int usc;
	char uid[20];
	struct ndctl_dimm *next;
};

struct ndctl_interleave_set {
	struct ndctl_dimm dimms[DIMM_MAX];
	struct ndctl_interleave_set *next;
};

struct ndctl_region {
	struct ndctl_namespace namespaces[NAMESPACES_MAX];
	struct ndctl_interleave_set *set;
	struct ndctl_region *next;
};

struct ndctl_bus {
	struct ndctl_region regions[REGION_MAX];
	struct ndctl_interleave_set sets[SETS_MAX];
	struct ndctl_bus *next;
};

struct ndctl_ctx {
	struct ndctl_bus buses[BUS_MAX];
} Ctx;

static int Dimm_iter;
int Usc[DIMM_MAX * BUS_MAX] = {1,2,3,4};
char *Uids[DIMM_MAX * BUS_MAX] = {"ffff-aa-11-ABCD",
				  "ffff-aa-22-ABCD",
				  "ffff-aa-33-ABCD",
				  "ffff-aa-44-ABCD"};
static int Namespaces_iter;
enum namespace_type Types[BUS_MAX * REGION_MAX * NAMESPACES_MAX] = {
	BLOCK, BLOCK, PFN, PFN, DEVDAX, DEVDAX,	BTT, BTT};

static int Names_iter;
char *Names[BUS_MAX * REGION_MAX * NAMESPACES_MAX * DEVDAX_MAX] = {
	"pmem1", "pmem2", "pnf1", "pnf2", "dax1", "dax2", "dax3", "dax4",
	"btt1", "btt2"
};

static void
init_region(struct ndctl_region *reg)
{
	for (int n = 0; n < NAMESPACES_MAX; n++) {
		struct ndctl_namespace *space =
			&reg->namespaces[n];
		space->next = n + 1 < NAMESPACES_MAX ?
			space + 1 : NULL;
		space->type = Types[Namespaces_iter++];

		char *name;
		struct daxctl_region reg;
		switch (space->type) {
		case DEVDAX:
			reg = space->dax.region;
			for (int d = 0 ; d < DEVDAX_MAX; d++) {
				strcpy(reg.devdax[d].name, Names[Names_iter++]);
			}
			break;
		case BTT:
			name = space->btt.name;
		case BLOCK:
			name = space->name;
		case PFN:
			name = space->pfn.name;
			strcpy(name, Names[Names_iter++]);
			break;
		}
	}
}

static void
init_bus(struct ndctl_bus *bus)
{
	for (int r = 0; r < REGION_MAX; r++) {
		struct ndctl_region *reg = &bus->regions[r];
		reg->next = r + 1 < REGION_MAX ? reg + 1 : NULL;
		reg->set = &bus->sets[r];
		init_region(reg);
	}

	for (int i = 0; i < SETS_MAX; i++) {
		struct ndctl_interleave_set *set =
			&bus->sets[i];
		set->next = i + 1 < SETS_MAX ? set + 1 : NULL;
		for (int d = 0; i < DIMM_MAX; i++) {
			struct ndctl_dimm *dimm = &set->dimms[d];
			dimm->next = d + 1 < DIMM_MAX ? dimm + 1 : NULL;
			dimm->usc = Usc[Dimm_iter];
			strcpy(dimm->uid, Uids[Dimm_iter]);
			Dimm_iter++;
		}
	}
}

int ndctl_new(struct ndctl_ctx **ctx)
{
	Dimm_iter = 0;
	for (int b = 0; b < BUS_MAX; b++) {
		struct ndctl_bus *bus = &Ctx.buses[b];
		bus->next = b + 1 < BUS_MAX ? bus + 1 : NULL;
		init_bus(bus);
	}

	*ctx = &Ctx;
	return 0;
}

struct ndctl_ctx *ndctl_unref(struct ndctl_ctx *ctx)
{
	return NULL;
}

struct ndctl_interleave_set *ndctl_interleave_set_get_first(struct ndctl_bus *bus)
{
	return &bus->sets[0];
}

struct ndctl_interleave_set *ndctl_interleave_set_get_next(struct ndctl_interleave_set *iset)
{
	return iset->next;
}

struct ndctl_interleave_set *ndctl_region_get_interleave_set(struct ndctl_region *region)
{
	return region->set;
}

long long ndctl_dimm_get_dirty_shutdown(struct ndctl_dimm *dimm)
{
	return dimm->usc;
}

const char *ndctl_btt_get_block_device(struct ndctl_btt *btt)
{
	return btt->name;
}

struct ndctl_bus *ndctl_bus_get_first(struct ndctl_ctx *ctx)
{
	return &ctx->buses[0];
}

struct ndctl_bus *ndctl_bus_get_next(struct ndctl_bus *bus)
{
	return bus->next;
}

const char *ndctl_dimm_get_unique_id(struct ndctl_dimm *dimm)
{
	return dimm->uid;
}

struct ndctl_dimm *ndctl_interleave_set_get_first_dimm(
	struct ndctl_interleave_set *iset)
{
	return &iset->dimms[0];
}

struct ndctl_dimm *ndctl_interleave_set_get_next_dimm(
	struct ndctl_interleave_set *iset, struct ndctl_dimm *dimm)
{
	return dimm->next;
}

const char *ndctl_namespace_get_block_device(struct ndctl_namespace *ndns)
{
	return ndns->type == BLOCK ? ndns->name : NULL;
}

struct ndctl_btt *ndctl_namespace_get_btt(struct ndctl_namespace *ndns)
{
	return ndns->type == BTT ? &ndns->btt : NULL;
}

struct ndctl_dax *ndctl_namespace_get_dax(struct ndctl_namespace *ndns)
{
	return ndns->type == DEVDAX ? &ndns->dax : NULL;
}

struct ndctl_namespace *ndctl_namespace_get_first(struct ndctl_region *region)
{
	return &region->namespaces[0];
}

struct ndctl_namespace *ndctl_namespace_get_next(struct ndctl_namespace *ndns)
{
	return ndns->next;
}

struct ndctl_pfn *ndctl_namespace_get_pfn(struct ndctl_namespace *ndns)
{
	return ndns->type == PFN ? &ndns->pfn : NULL;
}

const char *ndctl_pfn_get_block_device(struct ndctl_pfn *pfn)
{
	return pfn->name;
}

struct ndctl_region *ndctl_region_get_first(struct ndctl_bus *bus)
{
	return &bus->regions[0];
}

struct ndctl_region *ndctl_region_get_next(struct ndctl_region *region)
{
	return region->next;
}

struct daxctl_region *ndctl_dax_get_daxctl_region(struct ndctl_dax *dax)
{
	return &dax->region;
}

const char *daxctl_dev_get_devname(struct daxctl_dev *dev)
{
	return dev->name;
}

struct daxctl_dev *daxctl_dev_get_first(struct daxctl_region *region)
{
	return &region->devdax[0];
}

struct daxctl_dev *daxctl_dev_get_next(struct daxctl_dev *dev)
{
	return dev->next;
}
