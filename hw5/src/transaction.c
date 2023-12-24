#include <stdlib.h>
#include <pthread.h>
#include <csapp.h>
#include <string.h>
#include <unistd.h>
#include <debug.h>

#include "transaction.h"
#include "data.h"

int id_count = 0;

/*
 * Initialize the transaction manager.
 */
void trans_init(void) {
  trans_list.prev = &trans_list;
  trans_list.next = &trans_list;
  // TRANSACTION *initial_trans = Malloc(sizeof(TRANSACTION));

  // initial_trans->id = 0;
  // initial_trans->refcnt = 0;
  // initial_trans->status = TRANS_PENDING;
  // initial_trans->waitcnt = 0;
  // initial_trans->next = NULL;
  // initial_trans->prev = NULL;

  // Sem_init(&initial_trans->sem, 0, 1);
  // int result = pthread_mutex_init(&initial_trans->mutex, NULL);

  // if (!result) {
    debug("Initialize transaction manager");
  // }

  // else {
  //   debug("Initialize transaction failed");
  // }
}

/*
 * Finalize the transaction manager.
 */
void trans_fini(void) {
  // sem_destroy(&cr->sem);
  // pthread_mutex_destroy(&cr->mutex);
  // Free(cr);
}

/*
 * Create a new transaction.
 *
 * @return  A pointer to the new transaction (with reference count 1)
 * is returned if creation is successful, otherwise NULL is returned.
 */
TRANSACTION *trans_create(void) {
  TRANSACTION *initial_trans = Malloc(sizeof(TRANSACTION));

  initial_trans->id = id_count;
  initial_trans->refcnt = 0;
  initial_trans->status = TRANS_PENDING;
  initial_trans->depends = NULL;
  initial_trans->waitcnt = 0;
  initial_trans->next = &trans_list;
  initial_trans->prev = trans_list.prev;
  trans_list.prev->next = initial_trans;
  trans_list.prev = initial_trans;

  Sem_init(&initial_trans->sem, 0, 0);
  int result = pthread_mutex_init(&initial_trans->mutex, NULL);

  if (!result) {
    debug("Create new transaction %d", initial_trans->id);
    trans_ref(initial_trans, "for newly created transaction");
    id_count++;
    return initial_trans;
  }

  else {
    debug("Initialize transaction failed");
    Free(initial_trans);
    return NULL;
  }
}

/*
 * Increase the reference count on a transaction.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the increase.
 * @return  The transaction pointer passed as the argument.
 */
TRANSACTION *trans_ref(TRANSACTION *tp, char *why) {
  pthread_mutex_lock(&tp->mutex);

  int temp = tp->refcnt;
  tp->refcnt = temp + 1;

  debug("Increase ref count on transaction %d (%d -> %d) %s", tp->id, temp, tp->refcnt, why);

  pthread_mutex_unlock(&tp->mutex);
  return tp;
}

/*
 * Decrease the reference count on a transaction.
 * If the reference count reaches zero, the transaction is freed.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the decrease.
 */
void trans_unref(TRANSACTION *tp, char *why) {
    pthread_mutex_lock(&tp->mutex);

    int temp = tp->refcnt;
    tp->refcnt = temp - 1;

    debug("Decrease ref count on transaction %d (%d -> %d) %s", tp->id, temp, tp->refcnt, why);

    pthread_mutex_unlock(&tp->mutex);

    if (tp->refcnt == 0) {
        // debug("Free blob %p [%s]", bp, bp->content);
        DEPENDENCY *depend_unref = tp->depends;

        if (depend_unref == NULL) {
          Free(tp->depends);
        }

        else {
          while (depend_unref->next != NULL) {
            depend_unref->trans->refcnt -= 1;
            depend_unref->trans = NULL;
            Free(depend_unref);
            depend_unref = depend_unref->next;
          }
        }

        tp->next->prev = tp->prev;
        tp->prev->next = tp->next;

        sem_destroy(&tp->sem);
        pthread_mutex_destroy(&tp->mutex);
        Free(tp);
    }
}

/*
 * Add a transaction to the dependency set for this transaction.
 *
 * @param tp  The transaction to which the dependency is being added.
 * @param dtp  The transaction that is being added to the dependency set.
 */
void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp) {
  // dtp = dependee

  DEPENDENCY *dependent = Malloc(sizeof(DEPENDENCY));
  dependent->trans = dtp;
  dependent->next = NULL;

  // check duplication
  int duplicate = 0;
  DEPENDENCY *depend_while = tp->depends;

  while(1) {
    if (depend_while != NULL) {
      if (depend_while->trans == dtp) {
        Free(dependent);
        duplicate = 1;
      }
      break;
    }
    else
      break;

    depend_while = depend_while->next;
  }

  if (!duplicate) {
    DEPENDENCY *depend_check = tp->depends;

    if (depend_check == NULL) {
      debug("Make transaction %d dependent on transaction %d", tp->id, dtp->id);
      tp->depends = dependent;
      // debug("did you add dependency? %p", tp->depends);
      trans_ref(dtp, "for transaction in dependency");
    }

    else {
      while(1) {
        if (depend_check->next == NULL) {
          debug("Make transaction %d dependent on transaction %d", tp->id, dtp->id);
          depend_check->next = dependent;
          trans_ref(dtp, "for transaction in dependency");
          break;
        }

        depend_check = depend_check->next;
      }
    }
  }
}

/*
 * Try to commit a transaction.  Committing a transaction requires waiting
 * for all transactions in its dependency set to either commit or abort.
 * If any transaction in the dependency set abort, then the dependent
 * transaction must also abort.  If all transactions in the dependency set
 * commit, then the dependent transaction may also commit.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be committed.
 * @return  The final status of the transaction: either TRANS_ABORTED,
 * or TRANS_COMMITTED.
 */
TRANS_STATUS trans_commit(TRANSACTION *tp) {
  debug("Transaction %d trying to commit", tp->id);

  DEPENDENCY *depend_commit = tp->depends;

  if (tp->status == TRANS_ABORTED) {
    debug("abort in commit");
    return trans_abort(tp);
  }

  if (tp->status == TRANS_COMMITTED) {
    debug("commit in commit");
    return TRANS_COMMITTED;
  }

  while (depend_commit != NULL) {
    debug("Transaction %d checking status of dependency %d", tp->id, tp->depends->trans->id);
    if(depend_commit->trans->status == TRANS_ABORTED) {
      return trans_abort(tp);
    }

    depend_commit->trans->waitcnt += 1;

    debug("Transaction %d waiting for dependency %d", tp->id, depend_commit->trans->id);

    P(&depend_commit->trans->sem);

    depend_commit = depend_commit->next;
  }

  while (tp->waitcnt != 0) {
    V(&tp->sem);
    tp->waitcnt -= 1;
  }

  DEPENDENCY *depend_check_abort = tp->depends;

  while (depend_check_abort != NULL) {
    debug("Transaction %d checking status of dependency %d", tp->id, tp->depends->trans->id);
    if(depend_check_abort->trans->status == TRANS_ABORTED) {
      return trans_abort(tp);
    }
    depend_check_abort = depend_check_abort->next;
  }

  debug("Transaction %d commits", tp->id);
  debug("Release %d waiters dependent on transaction %d", tp->waitcnt, tp->id);
  trans_unref(tp, "for attempting to commit transaction");

  return TRANS_COMMITTED;
}

/*
 * Abort a transaction.  If the transaction has already committed, it is
 * a fatal error and the program crashes.  If the transaction has already
 * aborted, no change is made to its state.  If the transaction is pending,
 * then it is set to the aborted state, and any transactions dependent on
 * this transaction must also abort.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be aborted.
 * @return  TRANS_ABORTED.
 */
TRANS_STATUS trans_abort(TRANSACTION *tp) {
   while (tp->waitcnt != 0) {
    V(&tp->sem);
    tp->waitcnt -= 1;
  }
  if (tp->status == TRANS_COMMITTED)
    abort();
  if (tp->status == TRANS_ABORTED)
    return TRANS_ABORTED;

  tp->status = TRANS_ABORTED;
  debug("Transaction %d aborts", tp->id);
  trans_unref(tp, "for aborting transaction");

  return TRANS_ABORTED;
}

/*
 * Get the current status of a transaction.
 * If the value returned is TRANS_PENDING, then we learn nothing,
 * because unless we are holding the transaction mutex the transaction
 * could be aborted at any time.  However, if the value returned is
 * either TRANS_COMMITTED or TRANS_ABORTED, then that value is the
 * stable final status of the transaction.
 *
 * @param tp  The transaction.
 * @return  The status of the transaction, as it was at the time of call.
 */
TRANS_STATUS trans_get_status(TRANSACTION *tp) {
  return tp->status;
}

/*
 * Print information about a transaction to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 *
 * @param tp  The transaction to be shown.
 */
void trans_show(TRANSACTION *tp) {
  return;
}

/*
 * Print information about all transactions to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 */
void trans_show_all(void) {
  return;
}
