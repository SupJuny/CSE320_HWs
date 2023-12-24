#include <stdlib.h>
#include <pthread.h>
#include <csapp.h>
#include <string.h>
#include <unistd.h>
#include <debug.h>

#include "transaction.h"
#include "data.h"

/*
 * Create a blob with given content and size.
 * The content is copied, rather than shared with the caller.
 * The returned blob has one reference, which becomes the caller's
 * responsibility.
 *
 * @param content  The content of the blob.
 * @param size  The size in bytes of the content.
 * @return  The new blob, which has reference count 1.
 */
BLOB *blob_create(char *content, size_t size) {
    if (content == NULL)
        return NULL;

    BLOB *new_blob = Malloc(sizeof(BLOB));
    new_blob->content = strndup(content, size);
    new_blob->prefix = strndup(content, size);
    new_blob->size = size;
    new_blob->refcnt = 0;

    int result = pthread_mutex_init(&new_blob->mutex, NULL);

    if (!result) {
        debug("Create blob with content %p, size %d -> %p", new_blob->content, (int)new_blob->size, new_blob);
        blob_ref(new_blob, "for newly created blob");
        return new_blob;
    }

    else {
        debug("creation failed");
        return NULL;
    }
}

/*
 * Increase the reference count on a blob.
 *
 * @param bp  The blob.
 * @param why  Short phrase explaining the purpose of the increase.
 * @return  The blob pointer passed as the argument.
 */
BLOB *blob_ref(BLOB *bp, char *why) {
    pthread_mutex_lock(&bp->mutex);

    int temp = bp->refcnt;
    bp->refcnt = temp + 1;

    debug("Increase reference count on blob %p [%s] (%d -> %d) %s", bp, bp->content, temp, bp->refcnt, why);

    pthread_mutex_unlock(&bp->mutex);
    return bp;
}

/*
 * Decrease the reference count on a blob.
 * If the reference count reaches zero, the blob is freed.
 *
 * @param bp  The blob.
 * @param why  Short phrase explaining the purpose of the decrease.
 */
void blob_unref(BLOB *bp, char *why) {
    pthread_mutex_lock(&bp->mutex);

    int temp = bp->refcnt;
    bp->refcnt = temp - 1;

    debug("Decrease reference count on blob %p [%s] (%d -> %d) %s", bp, bp->content, temp, bp->refcnt, why);

    pthread_mutex_unlock(&bp->mutex);

    if (bp->refcnt == 0) {
        pthread_mutex_destroy(&bp->mutex);
        debug("Free blob %p [%s]", bp, bp->content);
        Free(bp->content);
        Free(bp->prefix);
        Free(bp);
    }
}

/*
 * Compare two blobs for equality of their content.
 *
 * @param bp1  The first blob.
 * @param bp2  The second blob.
 * @return 0 if the blobs have equal content, nonzero otherwise.
 */
int blob_compare(BLOB *bp1, BLOB *bp2) {
    return strcmp(bp1->content, bp2->content);
}

/*
 * Hash function for hashing the content of a blob.
 *
 * @param bp  The blob.
 * @return  Hash of the blob.
 */
int blob_hash(BLOB *bp) {
    int i = 0;
    int j = strlen(bp->content);

    for (int k = 0; k < j; k++) {
        i += bp->content[k]*k;
    }

    return i % 999;
}

/*
 * Create a key from a blob.
 * The key inherits the caller's reference to the blob.
 *
 * @param bp  The blob.
 * @return  the newly created key.
 */
KEY *key_create(BLOB *bp) {
    // debug("get into key_create function");
    KEY *new_key = Malloc(sizeof(KEY));

    new_key->hash = blob_hash(bp);
    new_key->blob = bp;

    debug("Create key from blob %p -> %p [%s]", bp, new_key, bp->content);

    return new_key;
}

/*
 * Dispose of a key, decreasing the reference count of the contained blob.
 * A key must be disposed of only once and must not be referred to again
 * after it has been disposed.
 *
 * @param kp  The key.
 */
void key_dispose(KEY *kp) {
    debug("Dispose of key %p [%s]", kp, kp->blob->content);
    blob_unref(kp->blob, "for blob in key");
    Free(kp);
}

/*
 * Compare two keys for equality.
 *
 * @param kp1  The first key.
 * @param kp2  The second key.
 * @return  0 if the keys are equal, otherwise nonzero.
 */
int key_compare(KEY *kp1, KEY *kp2) {
    if (kp1->hash != kp2->hash)
        return -1;
    else
        return blob_compare(kp1->blob, kp2->blob);
}

/*
 * Create a version of a blob for a specified creator transaction.
 * The version inherits the caller's reference to the blob.
 * The reference count of the creator transaction is increased to
 * account for the reference that is stored in the version.
 *
 * @param tp  The creator transaction.
 * @param bp  The blob.
 * @return  The newly created version.
 */
VERSION *version_create(TRANSACTION *tp, BLOB *bp) {
    VERSION *new_version = Malloc(sizeof(VERSION));

    if (bp == NULL) {
        debug("Create NULL version for transaction %d -> %p", tp->refcnt, new_version);
    }
    else {
        debug("Create version of blob %p [%s] for transaction %d -> %p", bp, bp->content, tp->refcnt, new_version);
    }

    new_version->creator = tp;
    new_version->blob = bp;
    new_version->next = NULL;
    new_version->prev = NULL;

    trans_ref(new_version->creator, "as creator of version");

    return new_version;
}

/*
 * Dispose of a version, decreasing the reference count of the
 * creator transaction and contained blob.  A version must be
 * disposed of only once and must not be referred to again once
 * it has been disposed.
 *
 * @param vp  The version to be disposed.
 */
void version_dispose(VERSION *vp) {
    debug("Dispose of version %p", vp);
    trans_unref(vp->creator, "as creator of version");
    if (vp->blob != NULL)
        blob_unref(vp->blob, "for blob in version");

    Free(vp);
}

