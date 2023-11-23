// vim: set colorcolumn=85
// vim: fdm=marker

#include "koh_destral_ecs.h"
#include "koh_set.h"
#include "munit.h"
#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "koh_common.h"
#include "koh_metaloader.h"

static const bool verbose = false;

struct Vectors {
    Vector2 *vecs;
    int     num;
};

static koh_SetAction iter_set_remove_all(
    const void *key, int key_len, void *udata
) {
    return koh_SA_remove_next;
}

static koh_SetAction iter_set_cmp(const void *key, int key_len, void *udata) {
    struct Vectors *lines = udata;

    for (int i = 0; i < lines->num; ++i) {
        if (!memcmp(&lines->vecs[i], key, key_len)) {
            return koh_SA_next;
        }
    }

    //printf("iter_set: key %s not found in each itertor\n", key);
    munit_assert(false);

    return koh_SA_next;
}

struct NumbersCtx {
    uint32_t    *arr;
    int         num;
};

static koh_SetAction iter_each(const void *key, int key_len, void *udata) {
    struct NumbersCtx *ctx = udata;
    for (int i = 0; i < ctx->num; ++i) {
        if (ctx->arr[i] == *((uint32_t*)key))
            return koh_SA_next;
    }
    munit_assert(false);
    return koh_SA_next;
}

static MunitResult test_each(
    const MunitParameter params[], void* data
) {
    uint32_t nums[] = {
        1, 3, 5, 7, 11, 13, 15,
    };
    int nums_num = sizeof(nums) / sizeof(nums[0]);

    koh_Set *set = set_new();
    for (int i = 0; i< nums_num; ++i) {
        set_add(set, &nums[i], sizeof(nums[i]));
    }

    struct NumbersCtx ctx = {
        .arr = nums, 
        .num = nums_num,
    };
    set_each(set, iter_each, &ctx);
    set_free(set);

    return MUNIT_OK;
}

void test_each_view_int_arr(int *arr, int arr_len) {
    assert(arr);

    int *examples = arr;
    int examples_num = arr_len;

    bool examples_exists[examples_num];
    memset(examples_exists, 0, sizeof(examples_exists));

    koh_Set *set = set_new();

    for (int j = 0; j < examples_num; ++j) {
        set_add(set, &examples[j], sizeof(int));
    }

    bool use_print = verbose;
    if (use_print)
        printf("set_size = %d\n", set_size(set));

    bool verbose_printing = koh_set_view_verbose;
    struct koh_SetView v = set_each_begin(set);
    koh_set_view_verbose = false;
    while (set_each_valid(&v)) {
        const int *key = set_each_key(&v);
        if (use_print)
            printf("key %p\n", key);
        if (key) {
            if (use_print)
                printf("%d ", *key);
        }
        set_each_next(&v);
    }
    if (use_print)
        printf("\n");
    koh_set_view_verbose = verbose_printing;

    /*
    индекс      taken
    ------------------
    0           false           
    1           false
    2           false
    3           true        <- set_each_begin()
    4           false
    5           true        <- set_each_next()
     */

    for (struct koh_SetView v = set_each_begin(set); 
            // может быть вызван много раз, (инвариат цикла?)
            set_each_valid(&v);
            // двигает состояние цикла к следующей записи
            set_each_next(&v)) {
        const int *key = set_each_key(&v);
        munit_assert_ptr_not_null(key);

        for (int i = 0; i < examples_num; i++) {
            if (examples[i] == *key) {
                examples_exists[i] = true;
            }
        }

        munit_assert_int(set_each_key_len(&v), ==, sizeof(int));
    }

    int exists_num = 0;
    for (int i = 0; i < examples_num; i++) {
        if (verbose)
            printf("[%d] = %d ", i, examples[i]);
        if (examples_exists[i]) {
            exists_num++;
            koh_term_color_set(KOH_TERM_MAGENTA);
            if (verbose)
                printf("✔, ");
            koh_term_color_reset();
        } else {
            koh_term_color_set(KOH_TERM_RED);
            if (verbose)
                printf("🞬, ");
            koh_term_color_reset();
        }
    }

    if (verbose)
        printf("exists_num %d, examples_num %d\n", exists_num, examples_num);
    munit_assert(exists_num == examples_num);

    set_free(set);
}

static MunitResult test_each_view(
    const MunitParameter params[], void* data
) {
    int arr_len;

    {
        int arr1[] = { 0, 1, 2, 3, 4, 5};
        arr_len = sizeof(arr1) / sizeof(arr1[0]);;
        test_each_view_int_arr(arr1, arr_len);
    }

    {
        int arr2[] = { -5, 5, -6, 6,};
        arr_len = sizeof(arr2) / sizeof(arr2[0]);;
        test_each_view_int_arr(arr2, arr_len);
    }

    {
        int arr3[] = { 111 };
        arr_len = sizeof(arr3) / sizeof(arr3[0]);;
        test_each_view_int_arr(arr3, arr_len);
    }

    {
        int arr4[] = { };
        // 11, 13, 5
        arr_len = sizeof(arr4) / sizeof(arr4[0]);;
        test_each_view_int_arr(arr4, arr_len);
    }

    return MUNIT_OK;
}

static MunitResult test_compare_1(
    const MunitParameter params[], void* data
) {

    Vector2 vecs1[] = {
        { 1.,    0. },
        { 12.,   0. },
        { 0.1,   0. },
        { 1.3, -0.1 },
        { 0.,   0.5 },
        { 14,    0. },
    };
    int vecs1_num = sizeof(vecs1) / sizeof(vecs1[0]);

    Vector2 vecs2[] = {
        { 1.,    0.1 },
        { 12.,   0. },
        { 0.1,   0.1 },
        { 1.3, -0.1 },
        { 0.,   0.5 },
        { 14,    0. },
    };
    int vecs2_num = sizeof(vecs2) / sizeof(vecs2[0]);

    koh_Set *set1 = set_new();
    koh_Set *set2 = set_new();
    koh_Set *set3 = set_new();

    /*
     set1 == set3
     set1 != set2
     */

    for (int i = 0; i< vecs1_num; ++i) {
        set_add(set1, &vecs1[i], sizeof(vecs1[0]));
    }

    for (int i = 0; i< vecs2_num; ++i) {
        set_add(set2, &vecs2[i], sizeof(vecs2[0]));
    }

    for (int i = 0; i< vecs1_num; ++i) {
        set_add(set3, &vecs1[i], sizeof(vecs1[0]));
    }

    munit_assert(set_compare(set1, set3));
    munit_assert(!set_compare(set1, set2));

    set_free(set1);
    set_free(set2);
    set_free(set3);

    return MUNIT_OK;
}

struct MetaObject {
    Rectangle   rect;
    char        name[128];
};

static koh_Set *control_set_alloc(struct MetaLoaderObjects objs) {
    koh_Set *set_control = set_new();
    assert(set_control);

    for (int i = 0; i < objs.num; ++i) {
        struct MetaObject mobject = {};
        strncpy(mobject.name, objs.names[i], sizeof(mobject.name));
        mobject.rect = objs.rects[i];
        /*
        printf(
            "control_set_alloc: mobject '%s', %s\n",
            mobject.name, rect2str(mobject.rect)
        );
        */
        set_add(set_control, &mobject, sizeof(mobject));
    }

    return set_control;
}

static MunitResult test_compare_4_noeq(
    const MunitParameter params[], void* data
) {

    koh_Set *set1 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            "wheel1", "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0.01, 0, 0},
        },
        .num = 5,   // изменено количество
    });

    koh_Set *set2 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            "wheel1", "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    munit_assert_false(set_compare(set1, set2));

    if (set1)
        set_free(set1);
    if (set2)
        set_free(set2);

    return MUNIT_OK;
}

static MunitResult test_compare_3_noeq(
    const MunitParameter params[], void* data
) {

    koh_Set *set1 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            // Изменено здесь ↓
            "", 
            // Изменено здесь ↑ 

            "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    koh_Set *set2 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            "wheel1", "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    munit_assert_false(set_compare(set1, set2));

    if (set1)
        set_free(set1);
    if (set2)
        set_free(set2);

    return MUNIT_OK;
}

static MunitResult test_compare_2_noeq(
    const MunitParameter params[], void* data
) {

    koh_Set *set1 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            // Изменено здесь ↓
            "wh_el1", 
            // Изменено здесь ↑ 

            "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    koh_Set *set2 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            "wheel1", "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    munit_assert_false(set_compare(set1, set2));

    if (set1)
        set_free(set1);
    if (set2)
        set_free(set2);

    return MUNIT_OK;
}

static MunitResult test_compare_2_eq(
    const MunitParameter params[], void* data
) {

    koh_Set *set1 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            "wheel1", "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    koh_Set *set2 = control_set_alloc((struct MetaLoaderObjects) {
        .names = { 
            "wheel1", "mine"  , "wheel2", "wheel3", "wheel4", "wheel5", 
        },
        .rects = {
            { 0, 0, 100, 100, },
            { 2156, 264, 407, 418 },
            { 2, 20, 43, 43, },
            { 2000, 20, 43, 43, },
            { -20, 20, 43, 43, },
            { 0, 0, 0, 0},
        },
        .num = 6,
    });

    munit_assert(set_compare(set1, set2));

    if (set1)
        set_free(set1);
    if (set2)
        set_free(set2);

    return MUNIT_OK;
}

struct TestAddRemoveCtx {
    int     *examples;
    int     examples_num;
    int     examples_remove_value;
    bool    examples_remove_value_found;
    koh_Set *set;
};

static koh_SetAction iter_set_remove_by_value(
    const void *key, int key_len, void *udata
) {
    const int *key_value = key;

    if (!udata) {
        fprintf(stderr, "iter_set_check: udata == NULL\n");
        koh_trap();
    }

    struct TestAddRemoveCtx *ctx = udata;

    if (*key_value == ctx->examples_remove_value) {
        /*printf("iter_set_remove: found key %d\n", *key_value);*/
        ctx->examples_remove_value_found = true;
        return koh_SA_remove_break;
    }
    return koh_SA_next;
}

static koh_SetAction iter_set_check(
    const void *key, int key_len, void *udata
) {
    const int *key_value = key;
    if (!udata) {
        fprintf(stderr, "iter_set_check: udata == NULL\n");
        koh_trap();
    }

    struct TestAddRemoveCtx *ctx = udata;
    for (int i = 0; i < ctx->examples_num; ++i) {
        if (ctx->examples[i] == *key_value) {
            //printf("iter_set_check: found %d\n", *key_value);
            return koh_SA_next;
        }
    }

    if (verbose)
        printf("iter_set_remove: key_value = %d not found\n", *key_value);
    munit_assert(false);
    return koh_SA_next;
}

static MunitResult test_add_remove(
    const MunitParameter params[], void* data
) {
    int examples[] = {
        1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 20, 23, 24
    };
    koh_Set *set = set_new();

    int examples_num = sizeof(examples) / sizeof(examples[0]);
    struct TestAddRemoveCtx ctx = {
        .examples = calloc(sizeof(int), examples_num * 2),
        .examples_num = examples_num,
    };
    assert(ctx.examples);
    memcpy(ctx.examples, examples, sizeof(int) * examples_num);

    for (int i = 0; i < examples_num; i++) {
        set_add(set, &examples[i], sizeof(int));
    }

    set_each(set, iter_set_check, &ctx);

    set_remove(set, &ctx.examples[0], sizeof(int));
    ctx.examples[0] = 0;

    set_remove(set, &ctx.examples[1], sizeof(int));
    ctx.examples[1] = 0;

    set_each(set, iter_set_check, &ctx);

    ctx.examples[ctx.examples_num++] = 101;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 102;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 103;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 104;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 105;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    set_each(set, iter_set_check, &ctx);

    set_remove(set, &ctx.examples[5], sizeof(int));
    ctx.examples[5] = 0;

    set_remove(set, &ctx.examples[5], sizeof(int));
    ctx.examples[5] = 0;

    set_each(set, iter_set_check, &ctx);

    free(ctx.examples);
    set_free(set);
    return MUNIT_OK;
}

static void ctx_set_remove(struct TestAddRemoveCtx *ctx, int index) {
    assert(ctx);

    if (!ctx) {
        if (verbose)
            printf("set_remove_through: ctx == NULL\n");
        koh_trap();
    }

    assert(ctx->set);

    if (!ctx->examples) {
        if (verbose)
            printf("set_remove_through: ctx->examples == NULL\n");
        koh_trap();
    }

    assert(index < ctx->examples_num);
    if (ctx->examples)
        ctx->examples_remove_value = ctx->examples[index];
    else {
        printf("ctx_set_removce: ctx->examples == NULL\n");
        exit(EXIT_FAILURE);
    }
    //ctx->examples_remove_value_found = false;
    //set_each(ctx->set, iter_set_remove_by_value, ctx);
    set_remove(ctx->set, &ctx->examples[index], sizeof(int));
    ctx->examples[index] = 0;
    //if (!ctx->examples_remove_value_found) {
        //printf("ctx->examples_remove_value: %d\n", ctx->examples_remove_value);
        //munit_assert(ctx->examples_remove_value_found);
    //}
}

static void ctx_set_remove_through(struct TestAddRemoveCtx *ctx, int index) {
    assert(ctx);

    if (!ctx) {
        printf("set_remove_through: ctx == NULL\n");
        koh_trap();
    }

    assert(ctx->set);

    if (!ctx->examples) {
        printf("set_remove_through: ctx->examples == NULL\n");
        koh_trap();
    }

    assert(index < ctx->examples_num);
    ctx->examples_remove_value = ctx->examples[index];
    ctx->examples[index] = 0;
    ctx->examples_remove_value_found = false;
    set_each(ctx->set, iter_set_remove_by_value, ctx);
    if (!ctx->examples_remove_value_found) {
        if (verbose)
            printf(
                "ctx->examples_remove_value: %d\n",
                ctx->examples_remove_value
            );
        munit_assert(ctx->examples_remove_value_found);
    }
}

static MunitResult test_add_remove_each(
    const MunitParameter params[], void* data
) {

    int examples[] = {
        1,  // 0
        3,  // 1
        4,  // 2
        5,  // 3
        6,  // 4
        7,  // 5
        8,  // 6
        9,  // 7
        10, // 8
        11, // 9
        20, // 10
        23, // 11
        24  // 12
    };

    koh_Set *set = set_new();
    int examples_num = sizeof(examples) / sizeof(examples[0]);
    int examples_cap = examples_num * 10;
    struct TestAddRemoveCtx ctx = {
        .examples = calloc(sizeof(int), examples_cap),
        .examples_num = examples_num,
        .set = set,
    };
    assert(ctx.examples);
    memcpy(ctx.examples, examples, sizeof(int) * examples_num);

    for (int i = 0; i < examples_num; i++) {
        int example_value = examples[i];
        if (verbose)
            printf("%d ", example_value);
        set_add(set, &example_value, sizeof(int));
    }
    if (verbose) {
        printf("\n");
        printf("set_size(set) %d\n", set_size(set));
    }
    munit_assert_int(set_size(set), !=, 0);
    set_each(set, iter_set_check, &ctx);

    ctx_set_remove_through(&ctx, 0);
    ctx_set_remove_through(&ctx, 10);
    ctx_set_remove_through(&ctx, 12);
    ctx_set_remove_through(&ctx, 5);
    ctx_set_remove_through(&ctx, 6);
    ctx_set_remove_through(&ctx, 7);

    set_each(set, iter_set_check, &ctx);

    bool use_print = false;
    if (use_print)
        printf("new elements were added: ");
    for (int j = examples_num; j < examples_num * 2; j++) {
        ctx.examples[ctx.examples_num++] = 100 + j;
        if (use_print)
            printf("%d ", ctx.examples[ctx.examples_num - 1]);
        set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));
    }
    if (use_print)
        printf("\n");

    // XXX: Что показывают эти цифры?
    if (verbose) {
        for (int i = 0; i < ctx.examples_num; i++) {
            int example_value = ctx.examples[i];
            printf("%d ", example_value);
        }
        printf("\n");
    }

    for (int j = examples_num - 1; j < examples_num / 2; j--) {
        if (verbose)
            printf("j %d, ctx.examples[%d] = %d\n", j, j, ctx.examples[j]);
        if (ctx.examples[j])
            ctx_set_remove_through(&ctx, j);
    }

    ctx.examples[ctx.examples_num++] = 1102;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 1103;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 1104;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    ctx.examples[ctx.examples_num++] = 1105;
    set_add(set, &ctx.examples[ctx.examples_num - 1], sizeof(int));

    set_each(set, iter_set_check, &ctx);

    ctx_set_remove(&ctx, ctx.examples_num - 1);
    ctx.examples_num--;
    ctx_set_remove(&ctx, ctx.examples_num - 1);
    ctx.examples_num--;
    ctx_set_remove(&ctx, ctx.examples_num - 1);
    ctx.examples_num--;
    ctx_set_remove(&ctx, ctx.examples_num - 1);
    ctx.examples_num--;

    if (verbose) {
        for (int i = 0; i < ctx.examples_num; i++) {
            printf("%d ", ctx.examples[i]);
        }
        printf("\n");
    }

    set_each(set, iter_set_check, &ctx);

    free(ctx.examples);
    set_free(set);
    return MUNIT_OK;
}

static MunitResult test_new_add_exist_free(
    const MunitParameter params[], void* data
) {
    koh_Set *set = set_new();
    munit_assert_ptr_not_null(set);

    Vector2 vecs[] = {
        { 1.,    0. },
        { 12.,   0. },
        { 0.1,   0. },
        { 1.3, -0.1 },
        { 0.,   0.5 },
        { 14,    0. },
    };

    Vector2 other_vecs[] = {
        { -1.,   0. },
        { 12.,  NAN },
        { 0.1,  0.1 },
        { 1.3,  0.1 },
        { 0.,  0.51 },
        { -14,   0. },
    };

    int vecs_num = sizeof(vecs) / sizeof(vecs[0]);
    int other_vecs_num = sizeof(other_vecs) / sizeof(other_vecs[0]);

    for (int i = 0; i< vecs_num; ++i) {
        set_add(set, &vecs[i], sizeof(vecs[0]));
    }

    for (int i = 0; i< vecs_num; ++i) {
        munit_assert(set_exist(set, &vecs[i], sizeof(vecs[0])));
    }

    for (int i = 0; i< other_vecs_num; ++i) {
        munit_assert(!set_exist(set, &other_vecs[i], sizeof(other_vecs[0])));
    }

    struct Vectors vectors_ctx = {
        //.vecs = (Vector2**)vecs, 
        .vecs = vecs, 
        .num = vecs_num,
    };
    // проверка всех ключей
    set_each(set, iter_set_cmp, &vectors_ctx);

    // удаление все ключей
    set_each(set, iter_set_remove_all, &vectors_ctx);

    Vector2 pi_vec = { M_PI, M_PI };
    set_add(set, &pi_vec, sizeof(Vector2));

    for (int i = 0; i< vecs_num; ++i) {
        munit_assert(!set_exist(set, &vecs[i], sizeof(Vector2)));
    }

    munit_assert(set_exist(set, &pi_vec, sizeof(Vector2)));
    set_clear(set);
    munit_assert(!set_exist(set, &pi_vec, sizeof(Vector2)));

    set_free(set);
    return MUNIT_OK;
}

static MunitTest test_suite_tests[] = {
  {
    (char*) "/each_view",
    test_each_view,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/add_remove_each",
    test_add_remove_each,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/add_remove",
    test_add_remove,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/new_add_exist_free",
    test_new_add_exist_free,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/each",
    test_each,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/compare_1",
    test_compare_1,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/compare_2_eq",
    test_compare_2_eq,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/compare_2_noeq",
    test_compare_2_noeq,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/compare_3_noeq",
    test_compare_3_noeq,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  {
    (char*) "/compare_4_noeq",
    test_compare_4_noeq,
    NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL
  },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = {
  (char*) "set", test_suite_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE
};

int main(int argc, char **argv) {
    return munit_suite_main(&test_suite, (void*) "µnit", argc, argv);
}
