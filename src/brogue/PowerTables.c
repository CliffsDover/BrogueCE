/*
 *  PowerTables.c
 *  Brogue
 *
 *  Created by Brian Walker on 4/9/17.
 *  Copyright 2017. All rights reserved.
 *
 *  This file is part of Brogue.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rogue.h"

// As of v1.7.5, Brogue does not use floating-point math in any calculations
// that have an effect on substantive gameplay. The two operations that were
// annoying to convert were sqrt() (handled by an open source fixed point sqrt
// implementation in Sqrt.c) and especially pow(). I could not find a fixed point
// pow implementation that was good enough for the wide range of fractional bases
// and exponents. Fortunately, all uses of pow() involved a fixed base and an exponent
// that varied by increments of at least 0.25 (or really 0.1 for armor calculations but
// I fudged that), or a fixed exponent and base that varied similarly. The one exception
// were runic weapon activation chances, which modified the base by the damage of the weapon
// and varied the exponent by the level, but I moved the damage modification into the exponent
// without much affecting the results. So now pow() has been replaced by lookup tables.
// Hopefully this will help with out of sync errors for saved games and recordings...

// game data formulae:

short wandDominate(creature *monst)             {return (((monst)->currentHP * 5 < (monst)->info.maxHP) ? 100 : \
                                                    max(0, 100 * ((monst)->info.maxHP - (monst)->currentHP) / (monst)->info.maxHP));}

short staffDamageLow(real enchant)              {return ((int)fp_trunc(enchant) + 2) * 3 / 4;}
short staffDamageHigh(real enchant)             {return (int)fp_trunc(enchant * 5) / 2 + 4;}
short staffDamage(real enchant)                 {return randClumpedRange(staffDamageLow(enchant), staffDamageHigh(enchant), (int)fp_trunc(enchant / 3) + 1);}
short staffBlinkDistance(real enchant)          {return (int)fp_trunc(enchant * 2) + 2;}
short staffHasteDuration(real enchant)          {return (int)fp_trunc(enchant * 4) + 2;}
short staffBladeCount(real enchant)             {return (int)fp_trunc(enchant * 1.5);}
short staffDiscordDuration(real enchant)        {return (int)fp_trunc(enchant * 4);}
short staffEntrancementDuration(real enchant)   {return (int)fp_trunc(enchant * 3);}
int   staffProtection(real enchant)             {return (int)fp_trunc(fp_pow(1.4, fp_trunc(enchant) - 2) * 130);}
int   staffPoison(real enchant)                 {return (int)fp_trunc(5 * fp_pow(1.3, clamp(fp_trunc(enchant) - 2, 0., 50.)));}

real ringWisdomMultiplier(real enchant)         {return fp_pow(1.3, min(27., fp_trunc(enchant)));}

short charmHealing(real enchant)                {return clamp((int)fp_trunc(20 * enchant), 0, 100);}
short charmShattering(real enchant)             {return (int)fp_trunc(enchant) + 4;}
short charmGuardianLifespan(real enchant)       {return (int)fp_trunc(enchant) * 2 + 4;}
short charmNegationRadius(real enchant)         {return (int)fp_trunc(enchant) * 3 + 1;}
int   charmProtection(real enchant)             {return (int)fp_trunc(150 * fp_pow(1.35, fp_trunc(enchant) - 1));}

short weaponParalysisDuration(real enchant)     {return max(2, (int)fp_trunc(enchant / 2) + 2);}
short weaponConfusionDuration(real enchant)     {return max(3, (int)fp_trunc(enchant * 1.5));}
short weaponForceDistance(real enchant)         {return max(4, (int)fp_trunc(enchant * 2) + 2);} // Depends on definition of staffBlinkDistance() above.
short weaponSlowDuration(real enchant)          {return max(3, (int)fp_trunc((fp_trunc(enchant) + 2) * (enchant + 2) / 3));}
short weaponImageCount(real enchant)            {return clamp((int)fp_trunc(enchant / 3), 1, 7);}
short weaponImageDuration(real enchant)         {return 3;}

short armorReprisalPercent(real enchant)        {return max(5, (int)fp_trunc(enchant * 5));}
short armorAbsorptionMax(real enchant)          {return max(1, (int)fp_trunc(enchant));}
short armorImageCount(real enchant)             {return clamp((int)fp_trunc(enchant / 3), 1, 5);}


short reflectionChance(real enchant) {
    enchant = fp_trunc(enchant * 4) / 4;
    return clamp(100 - fp_trunc(100 * fp_pow(0.85, enchant)), 1, 100);
}

long turnsForFullRegenInThousandths(real bonus) {
    real power = fp_pow(0.75, fp_trunc(bonus));
    return fp_trunc(1000 * TURNS_FOR_FULL_REGEN * power) + 2000;
}

real damageFraction(real netEnchant) {
    netEnchant = fp_trunc(netEnchant * 4) / 4;
    return fp_pow(1.065, netEnchant);
}

real accuracyFraction(real netEnchant) {
    netEnchant = fp_trunc(netEnchant * 4) / 4;
    return fp_pow(1.065, netEnchant);
}

real defenseFraction(real netDefense) {
    netDefense = fp_trunc(netDefense * 4) / 4;
    real x = netDefense / 10 + 20;
    x = fp_trunc(x * 4) / 4;
    x = fp_pow(0.877347265, x - 20);
    return x;
}

short charmEffectDuration(short charmKind, short enchant) {
    const short duration[NUMBER_CHARM_KINDS] = {
        3,  // Health
        20, // Protection
        7,  // Haste
        10, // Fire immunity
        5,  // Invisibility
        25, // Telepathy
        10, // Levitation
        0,  // Shattering
        18, // Guardian
        0,  // Teleportation
        0,  // Recharging
        0,  // Negation
    };
    const real increment[NUMBER_CHARM_KINDS] = {
        0,    // Health
        0,    // Protection
        1.20, // Haste
        1.25, // Fire immunity
        1.20, // Invisibility
        1.25, // Telepathy
        1.25, // Levitation
        0,    // Shattering
        0,    // Guardian
        0,    // Teleportation
        0,    // Recharging
        0,    // Negation
    };
    return fp_trunc(duration[charmKind] * fp_pow(increment[charmKind], enchant));
}

short charmRechargeDelay(short charmKind, short enchant) {
    const short duration[NUMBER_CHARM_KINDS] = {
        2500,   // Health
        1000,   // Protection
        800,    // Haste
        800,    // Fire immunity
        800,    // Invisibility
        800,    // Telepathy
        800,    // Levitation
        2500,   // Shattering
        700,    // Guardian
        920,    // Teleportation
        10000,  // Recharging
        2500,   // Negation
    };
    const real base[NUMBER_CHARM_KINDS] = {
        0.55, // Health
        0.60, // Protection
        0.65, // Haste
        0.60, // Fire immunity
        0.65, // Invisibility
        0.65, // Telepathy
        0.65, // Levitation
        0.60, // Shattering
        0.70, // Guardian
        0.60, // Teleportation
        0.55, // Recharging
        0.60, // Negation
    };
    short delay = charmEffectDuration(charmKind, enchant)
        + fp_trunc(duration[charmKind] * fp_pow(base[charmKind], enchant));
    return max(1, delay);
}

short runicWeaponChance(item *theItem, boolean customEnchantLevel, real enchantLevel) {
    const real effectChances[NUMBER_WEAPON_RUNIC_KINDS] = {
        0.84, // W_SPEED
        0.94, // W_QUIETUS
        0.93, // W_PARALYSIS
        0.85, // W_MULTIPLICITY
        0.86, // W_SLOWING
        0.89, // W_CONFUSION
        0.85, // W_FORCE
        0,    // W_SLAYING
        0,    // W_MERCY
        0};   // W_PLENTY
    real modifier;
    short runicType = theItem->enchant2;
    short chance, adjustedBaseDamage, tableIndex;

    if (runicType == W_SLAYING) {
        return 0;
    }
    if (runicType >= NUMBER_GOOD_WEAPON_ENCHANT_KINDS) { // bad runic
        return 15;
    }
    if (!customEnchantLevel) {
        enchantLevel = netEnchant(theItem);
    }

    // Innately high-damage weapon types are less likely to trigger runic effects.
    adjustedBaseDamage = (tableForItemCategory(theItem->category, NULL)[theItem->kind].range.lowerBound
                          + tableForItemCategory(theItem->category, NULL)[theItem->kind].range.upperBound) / 2;

    if (theItem->flags & ITEM_ATTACKS_STAGGER) {
        adjustedBaseDamage /= 2; // Normalize as though they attacked once per turn instead of every other turn.
    }
    //    if (theItem->flags & ITEM_ATTACKS_QUICKLY) {
    //      adjustedBaseDamage *= 2; // Normalize as though they attacked once per turn instead of twice per turn.
    //  } // Testing disabling this for balance reasons...

    modifier = 1.0 - min(0.99, (real)adjustedBaseDamage / 18);

    if (enchantLevel < 0) {
        chance = 0;
    } else {
        tableIndex = clamp((int)fp_trunc(enchantLevel * modifier * 4), 0, 200);
        chance = 100 - (short)fp_trunc(100 * fp_pow(effectChances[runicType], tableIndex * 0.25)); // good runic
    }

    // Slow weapons get an adjusted chance of 1 - (1-p)^2 to reflect two bites at the apple instead of one.
    if (theItem->flags & ITEM_ATTACKS_STAGGER) {
        chance = 100 - (100 - chance) * (100 - chance) / 100;
    }
    // Fast weapons get an adjusted chance of 1 - sqrt(1-p) to reflect one bite at the apple instead of two.
    if (theItem->flags & ITEM_ATTACKS_QUICKLY) {
        chance = 100 * fp_trunc(1.0 - fp_sqrt(1.0 - 0.01 * chance));
    }

    // The lowest percent change that a weapon will ever have is its enchantment level (if greater than 0).
    // That is so that even really heavy weapons will improve at least 1% per enchantment.
    chance = clamp(chance, max(1, (short)fp_trunc(enchantLevel)), 100);

    return chance;
}
