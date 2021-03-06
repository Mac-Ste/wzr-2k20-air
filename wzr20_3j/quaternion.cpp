/************************************************************************************************** 
    Obs�uga kwaternion�w 
   
    Aby zrealizowa� obr�t nale�y pomno�y� kwaternion orientacji przez kwaterion obrotu
    z lewej strony:
            
    qOrient = qRot*qOrient
    
    Uwaga! Zak�adam, �e kwaterniony qRot i qOrient s� d�ugo�ci 1, inaczej trzeba jeszcze je normalizowa�            
    
    Kwaternion mo�e tez reprezentowa� orientacj� obiektu, je�li za�o�ymy jak�� orientacj�
    pocz�tkow� np. obiekt skierowany w kierunku osi 0x, z normaln� zgodn� z 0y. W�wczas 
    kwaternion odpowiada obrotowi od po�o�enia poc��tkowego. 
****************************************************************************************************/
#include <stdlib.h>
//#include "vector3D.h"
#include "quaternion.h"


kwaternion::kwaternion(float x,float y,float z,float w)

{
	this->x=x;
	this->y=y;
	this->z=z;
	this->w=w;
}


kwaternion::kwaternion()
{
	x=0;
	y=0;
	z=0;
	w=1;
}

// mno�enie dw�ch kwaternion�w
kwaternion kwaternion::operator*(kwaternion q)
{
    double rx, ry, rz, rw;		// temp result	


	
	rx	= x*q.w + w*q.x - z*q.y + y*q.z;
	ry	= y*q.w + w*q.y - x*q.z + z*q.x;
	rz	= z*q.w + w*q.z - y*q.x + x*q.y;
	
	rw	= w*q.w - x*q.x - y*q.y - z*q.z;

	return kwaternion((float)rx,(float)ry,(float)rz,(float)rw);
}

// zamiana kwaterniona na reprezentacj� k�towo-osiow�
// dla wygody o� i k�t obrotu umieszczane s� w strukturze typu kwaternion 
kwaternion kwaternion::AsixAngle()
{
    Wektor3 v(x,y,z);

    if (v.dlugosc()==0) return kwaternion(0,0,0,1);
    else 
    {
        v=v.znorm();
		    float kat_obrotu = (w <= -1 ? 0 : (w >= 1 ? 0 : 2.0*acos(w)));
        return kwaternion(v.x,v.y,v.z,kat_obrotu);
    }
}

kwaternion kwaternion::operator~ ()
{
    return kwaternion(-x,-y,-z,w);
}
  
kwaternion kwaternion:: operator+= (kwaternion q)	// operator+= is used to add another Vector3D to this Vector3D.
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
    return *this;
}

kwaternion kwaternion:: operator+ (kwaternion q)	// operator+= is used to add another Vector3D to this Vector3D.
{
        return kwaternion(x+q.x,y+q.y,z+q.z,w+q.w);
}
  
kwaternion kwaternion:: operator- (kwaternion q)	// operator+= is used to add another Vector3D to this Vector3D.
{
        return kwaternion(x-q.x,y-q.y,z-q.z,w-q.w);
}  
  
kwaternion kwaternion::n() // licz normalna z obecnego wektora
{
float length;

	length=sqrt(x*x+y*y+z*z+w*w);
	if (length==0) return kwaternion(0,0,0,0);
	    else return kwaternion(x/length,y/length,z/length,w/length);
}

float kwaternion::l() // d�ugo�� 
{
  return sqrt(x*x+y*y+z*z+w*w);
}	

kwaternion kwaternion::operator* (float value)  // mnozenie razy skalar
{
	return kwaternion(x*value,y*value,z*value,w*value);
}

kwaternion kwaternion::operator/ (float value)  // dzielenie przez skalar
{
	return kwaternion(x/value,y/value,z/value,w/value);
}
	


/*  obr�t wektora z u�yciem kwaterniona obrotu - do sprawdzenia
macierz obrotu:
M =  [ w^2 + x^2 - y^2 - z^2       2xy - 2wz                   2xz + 2wy
       2xy + 2wz                   w^2 - x^2 + y^2 - z^2       2yz - 2wx
       2xz - 2wy                   2yz + 2wx                   w^2 - x^2 - y^2 + z^2 ]
*/
Wektor3 kwaternion::obroc_wektor(Wektor3 V)       
{ 
    Wektor3 Vo;

    kwaternion p = kwaternion(V.x,V.y,V.z,0);
    kwaternion q = kwaternion(x,y,z,w);
    kwaternion pp = q*p*~q;
    Vo.x = pp.x;Vo.y = pp.y; Vo.z = pp.z;

    return Vo;    
}

// zamiana obrotu wyra�onego reprezentacj� k�towo-osiow� na reprezentacj� kwaternionow�  
kwaternion AsixToQuat(Wektor3 v, float angle)
{
    float x,y,z,w;			// temp vars of vector
    double rad, scale;		// temp vars

	if (v.dlugosc()==0) return kwaternion();
	
	v = v.znorm();
	rad = angle;
	scale = sin(rad/2.0);


	w = (float)cos(rad/2.0);

	x = float(v.x * scale);
	y = float(v.y * scale);
	z = float(v.z * scale);

	return kwaternion(x,y,z,w);
}
