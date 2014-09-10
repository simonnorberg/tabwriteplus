/* Copyright (c) 1997-1999 Miller Puckette and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* [tabwriteplus~] is a modification of [tabwrite~] that outputs a bang when it reaches the end of an array.
 * Based on http://lists.puredata.info/pipermail/pd-list/2002-03/005025.html */

#include "m_pd.h"

/* ------------------------- tabwriteplus~ -------------------------- */

static t_class *tabwriteplus_tilde_class;

typedef struct _tabwriteplus_tilde
{
    t_object x_obj;
    t_outlet *x_bangout; /* Add bang outlet */
    int x_phase;
    int x_nsampsintab;
    t_word *x_vec;
    t_symbol *x_arrayname;
    t_float x_f;
} t_tabwriteplus_tilde;

static void tabwriteplus_tilde_tick(t_tabwriteplus_tilde *x);

static void *tabwriteplus_tilde_new(t_symbol *s)
{
    t_tabwriteplus_tilde *x = (t_tabwriteplus_tilde *)pd_new(tabwriteplus_tilde_class);
    x->x_phase = 0x7fffffff;
    x->x_arrayname = s;
    x->x_f = 0;
    x->x_bangout = outlet_new(&x->x_obj, &s_bang); /* Init outlet */
    return (x);
}

static void tabwriteplus_tilde_redraw(t_tabwriteplus_tilde *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a)
        bug("tabwriteplus_tilde_redraw");
    else garray_redraw(a);
    outlet_bang(x->x_bangout); /* Bang now! */
}

static t_int *tabwriteplus_tilde_perform(t_int *w)
{
    t_tabwriteplus_tilde *x = (t_tabwriteplus_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]), phase = x->x_phase, endphase = x->x_nsampsintab;
    if (!x->x_vec) goto bad;
    
    if (endphase > phase)
    {
        int nxfer = endphase - phase;
        t_word *wp = x->x_vec + phase;
        if (nxfer > n) nxfer = n;
        phase += nxfer;
        while (nxfer--)
        {
            t_sample f = *in++;
            if (PD_BIGORSMALL(f))
                f = 0;
            (wp++)->w_float = f;
        }
        if (phase >= endphase)
        {
            tabwriteplus_tilde_redraw(x);
            phase = 0x7fffffff;
        }
        x->x_phase = phase;
    }
    else x->x_phase = 0x7fffffff;
bad:
    return (w+4);
}

static void tabwriteplus_tilde_set(t_tabwriteplus_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name) pd_error(x, "tabwriteplus~: %s: no such array",
                                 x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &x->x_nsampsintab, &x->x_vec))
    {
        pd_error(x, "%s: bad template for tabwriteplus~", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabwriteplus_tilde_dsp(t_tabwriteplus_tilde *x, t_signal **sp)
{
    tabwriteplus_tilde_set(x, x->x_arrayname);
    dsp_add(tabwriteplus_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void tabwriteplus_tilde_bang(t_tabwriteplus_tilde *x)
{
    x->x_phase = 0;
}

static void tabwriteplus_tilde_start(t_tabwriteplus_tilde *x, t_floatarg f)
{
    x->x_phase = (f > 0 ? f : 0);
}

static void tabwriteplus_tilde_stop(t_tabwriteplus_tilde *x)
{
    if (x->x_phase != 0x7fffffff)
    {
        tabwriteplus_tilde_redraw(x);
        x->x_phase = 0x7fffffff;
    }
}

void tabwriteplus_tilde_setup(void)
{
    tabwriteplus_tilde_class = class_new(gensym("tabwriteplus~"),
                                     (t_newmethod)tabwriteplus_tilde_new, 0,
                                     sizeof(t_tabwriteplus_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabwriteplus_tilde_class, t_tabwriteplus_tilde, x_f);
    class_addmethod(tabwriteplus_tilde_class, (t_method)tabwriteplus_tilde_dsp,
                    gensym("dsp"), 0);
    class_addmethod(tabwriteplus_tilde_class, (t_method)tabwriteplus_tilde_set,
                    gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabwriteplus_tilde_class, (t_method)tabwriteplus_tilde_stop,
                    gensym("stop"), 0);
    class_addmethod(tabwriteplus_tilde_class, (t_method)tabwriteplus_tilde_start,
                    gensym("start"), A_DEFFLOAT, 0);
    class_addbang(tabwriteplus_tilde_class, tabwriteplus_tilde_bang);
}
