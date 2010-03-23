/****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 1998-2010 Inria. All rights reserved.
 *
 * This software was developed at:
 * IRISA/INRIA Rennes
 * Projet Lagadic
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * http://www.irisa.fr/lagadic
 *
 * This file is part of the ViSP toolkit.
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined by Trolltech AS of Norway and appearing in the file
 * LICENSE included in the packaging of this file.
 *
 * Licensees holding valid ViSP Professional Edition licenses may
 * use this file in accordance with the ViSP Commercial License
 * Agreement provided with the Software.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Contact visp@irisa.fr if any conditions of this licensing are
 * not clear to you.
 *
 * Description:
 * Le module "vwstack.c" contient les procedures de gestion
 * de la pile des points de vue (VieW STACK).
 *
 * Authors:
 * Jean-Luc CORRE
 *
 *****************************************************************************/

#include <visp/vpConfig.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include	<stdio.h>
#include	<string.h>

// #include	<varargs.h> /* modif Fedora
#include	<stdarg.h>

#include	<visp/vpMy.h>
#include	<visp/vpArit.h>
#include	<visp/vpView.h>
#include	<visp/vpVwstack.h>


#define	STACKSIZE	4


static	View_parameters	stack[STACKSIZE]= { DEFAULT_VIEW };
static	View_parameters	*sp		= stack;

/*
 * La procedure "fprintf_vwstack" affiche un parametre du sommet
 * de la pile des prises de vue.
 * Entree :
 * fp		Fichier de sortie.
 * argv		Argument a afficher.
 *		Si argv est nul, tous les parametres sont affiches.
 */
void
fprintf_vwstack (FILE *fp, char *argv)
{
	static	 char	proc_name[] = "fprintf_vwstack";

	if (argv == NULL || strcmp (argv, "type") == 0) {
		char	*typetoa;

		switch (sp->type) {
		case PARALLEL	:
			typetoa = "parallel";
			break;
		case PERSPECTIVE :
			typetoa = "perspective";
			break;
		default :
			typetoa = "unknown";
			break;
		}
		fprintf (fp, "(type\t%s)\n", typetoa);
		if (argv != NULL) return;
	}
	if (argv == NULL || strcmp (argv, "cop") == 0) {
		fprintf (fp, "(cop\t%.3f\t%.3f\t%.3f)\n",
			sp->cop.x, sp->cop.y, sp->cop.z);
		if (argv != NULL) return;
	}
	if (argv == NULL || strcmp (argv, "vrp") == 0) {
		fprintf (fp, "(vrp\t%.3f\t%.3f\t%.3f)\n",
			sp->vrp.x, sp->vrp.y, sp->vrp.z);
		if (argv != NULL) return;
	}
	if (argv == NULL || strcmp (argv, "vpn") == 0) {
		fprintf (fp, "(vpn\t%.3f\t%.3f\t%.3f)\n",
			sp->vpn.x, sp->vpn.y, sp->vpn.z);
		if (argv != NULL) return;
	}
	if (argv == NULL || strcmp (argv, "vup") == 0) {
		fprintf (fp, "(vup\t%.3f\t%.3f\t%.3f)\n",
			sp->vup.x, sp->vup.y, sp->vup.z);
		if (argv != NULL) return;
	}
	if (argv == NULL || strcmp (argv, "window") == 0) {
		fprintf (fp, "(window\t%.3f\t%.3f\t%.3f\t%.3f)\n",
			 sp->vwd.umin,sp->vwd.umax,sp->vwd.vmin,sp->vwd.vmax);
		if (argv != NULL) return;
	}
	if (argv == NULL || strcmp (argv, "depth") == 0) {
		fprintf (fp, "(depth\t%.3f\t%.3f)\n",
			sp->depth.front, sp->depth.back);
		if (argv != NULL) return;
	}
	if (argv != NULL)
		fprintf (stderr, "%s: argument unknown\n", proc_name);
}

/*
 * La procedure "get_vwstack" retourne le point de vue au sommet
 * de la pile des points de vue.
 * Sortie :
 * 		Pointeur sur le point de vue du sommet de la pile.
 */
View_parameters	*
get_vwstack ()
{
	return (sp);
}

/*
 * La procedure "load_vwstack" charge un point de vue au sommet
 * de la pile des points de vue.
 * Entree :
 * vp		Point de vue a charger.
 */
void
load_vwstack (View_parameters *vp)
{
	*sp = *vp;
}

/*
 * La procedure "pop_vwstack" depile le point de vue au sommet
 * de la pile des points de vue.
 */
void
pop_vwstack ()
{
	static	char	proc_name[] = "pop_vwstack";

	if (sp == stack) {
		fprintf (stderr, "%s: stack underflow\n", proc_name);
		return;
	}
	else	sp--;
}

/*
 * La procedure "push_vwstack" empile et duplique le point de vue au sommet
 * de la pile des points de vue.
 */
void
push_vwstack ()
{
	static	char	proc_name[] = "push_vwstack";

	if (sp == stack + STACKSIZE - 1) {
		fprintf (stderr, "%s: stack overflow\n", proc_name);
		return;
	}
	sp++;
	*sp = *(sp - 1);
}

/*
 * La procedure "swap_vwstack" echange les deux premiers elements
 * de la pile des points de vue.
 */
void
swap_vwstack ()
{
	View_parameters	*vp, tmp;

	vp = (sp == stack) ? sp + 1 : sp - 1;
	SWAP(*sp, *vp, tmp);
}

/*
 * La procedure "add_vwstack" modifie un agrument du point de vue au sommet
 * de la pile des points de vue.
 * Entree :
 * va_alist	Nom de l'argument a modifier suivi de ses parametres.
 */

void
add_vwstack (const char* path, ... )
//add_vwstack (va_alist)
// va_dcl
{
	static	char	proc_name[] = "add_vwstack";

       	va_list	ap;
	char	*argv;

	va_start (ap,path);
	argv = va_arg (ap, char *);
	if (strcmp (argv, "cop") == 0) {
 		/* initialise le centre de projection	*/
		SET_COORD3(sp->cop,
			(float) va_arg (ap, double),
			(float) va_arg (ap, double),
			(float) va_arg (ap, double));
	}
	else if (strcmp (argv, "depth") == 0) {
 		/* initialise les distances des plans de decoupage	*/
		sp->depth.front = (float) va_arg (ap, double);
		sp->depth.back  = (float) va_arg (ap, double);
	}
	else if (strcmp (argv, "type") == 0) {
 		/* initialise le type de projection	*/
		sp->type = (Type) va_arg (ap, int);
	}
	else if (strcmp (argv, "vpn") == 0) {
		/* initialise le vecteur normal au plan	*/
		float	x = (float) va_arg (ap, double);
		float	y = (float) va_arg (ap, double);
		float	z = (float) va_arg (ap, double);

		if (x == 0.0 && y == 0.0 && z == 0.0)
			fprintf (stderr, "%s: bad vpn\n", proc_name);
		else {
			SET_COORD3(sp->vpn,x,y,z);
		}
	}
	else if (strcmp (argv, "vrp") == 0) {
		/* initialise le vecteur de reference	*/
		SET_COORD3(sp->vrp,
			(float) va_arg (ap, double),
			(float) va_arg (ap, double),
			(float) va_arg (ap, double));
	}
	else if (strcmp (argv, "vup") == 0) {
		/* initialise le vecteur haut du plan	*/
		float	x = (float) va_arg (ap, double);
		float	y = (float) va_arg (ap, double);
		float	z = (float) va_arg (ap, double);

		if (x == 0.0 && y == 0.0 && z == 0.0)
			fprintf (stderr, "%s: bad vup\n", proc_name);
		else {
			SET_COORD3(sp->vup,x,y,z);
		}
	}
	else if (strcmp (argv, "window") == 0) {
		/* initialise la fenetre de projection	*/
		sp->vwd.umin = (float) va_arg (ap, double);
		sp->vwd.umax = (float) va_arg (ap, double);
		sp->vwd.vmin = (float) va_arg (ap, double);
		sp->vwd.vmax = (float) va_arg (ap, double);
	}
	else
		fprintf (stderr, "%s: bad argument\n", proc_name);
	va_end (ap);
}

#endif

